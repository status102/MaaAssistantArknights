import cv2
import numpy as np
from typing import Tuple, Optional


def register_images(
    image_a: np.ndarray,
    image_b: np.ndarray,
    roi_a: Tuple[int, int, int, int],
    roi_b: Tuple[int, int, int, int],
    method: str = 'SIFT',
    visualize: bool = False,
    output_path: Optional[str] = None
) -> Optional[np.ndarray]:
    """
    使用特征匹配将两张图像进行配准，返回从A到B的变换矩阵
    
    Args:
        image_a: 图像A (参考图像)
        image_b: 图像B (待配准图像)
        roi_a: 图像A的ROI区域 (x, y, width, height)
        roi_b: 图像B的ROI区域 (x, y, width, height)
        method: 特征检测方法，可选 'SIFT', 'ORB', 'AKAZE'
        visualize: 是否可视化匹配结果
        output_path: 可视化结果保存路径
    
    Returns:
        transformation_matrix: 3x3变换矩阵，将A中的点变换到B坐标系
                              如果匹配失败返回None
    """
    # 裁剪ROI区域
    x_a, y_a, w_a, h_a = roi_a
    x_b, y_b, w_b, h_b = roi_b
    
    crop_a = image_a[y_a:y_a+h_a, x_a:x_a+w_a]
    crop_b = image_b[y_b:y_b+h_b, x_b:x_b+w_b]
    
    # 转换为灰度图
    if len(crop_a.shape) == 3:
        gray_a = cv2.cvtColor(crop_a, cv2.COLOR_BGR2GRAY)
    else:
        gray_a = crop_a
        
    if len(crop_b.shape) == 3:
        gray_b = cv2.cvtColor(crop_b, cv2.COLOR_BGR2GRAY)
    else:
        gray_b = crop_b
    
    # 创建特征检测器
    if method == 'SIFT':
        detector = cv2.SIFT_create()
    elif method == 'ORB':
        detector = cv2.ORB_create(nfeatures=5000)
    elif method == 'AKAZE':
        detector = cv2.AKAZE_create()
    else:
        raise ValueError(f"不支持的特征检测方法: {method}")
    
    # 检测特征点和描述符
    kp_a, desc_a = detector.detectAndCompute(gray_a, None)
    kp_b, desc_b = detector.detectAndCompute(gray_b, None)
    
    if desc_a is None or desc_b is None:
        print("未检测到足够的特征点")
        return None
    
    # 特征匹配
    if method == 'ORB':
        matcher = cv2.BFMatcher(cv2.NORM_HAMMING, crossCheck=False)
    else:
        matcher = cv2.BFMatcher(cv2.NORM_L2, crossCheck=False)
    
    # 使用KNN匹配
    matches = matcher.knnMatch(desc_a, desc_b, k=2)
    
    # 应用比率测试筛选好的匹配
    good_matches = []
    for match_pair in matches:
        if len(match_pair) == 2:
            m, n = match_pair
            if m.distance < 0.75 * n.distance:
                good_matches.append(m)
    
    if len(good_matches) < 4:
        print(f"匹配点数量不足: {len(good_matches)}")
        return None
    
    # 提取匹配点的坐标
    pts_a = np.float32([kp_a[m.queryIdx].pt for m in good_matches])
    pts_b = np.float32([kp_b[m.trainIdx].pt for m in good_matches])
    
    # 调整坐标到原图坐标系
    pts_a += np.float32([x_a, y_a])
    pts_b += np.float32([x_b, y_b])
    
    # 计算单应性矩阵 (从A到B的变换)
    transformation_matrix, mask = cv2.findHomography(
        pts_a, pts_b, cv2.RANSAC, 5.0
    )
    
    if transformation_matrix is None:
        print("无法计算变换矩阵")
        return None
    
    # 统计内点数量
    inliers = np.sum(mask)
    print(f"匹配成功: {len(good_matches)} 个匹配点, {inliers} 个内点")
    
    # 可视化匹配结果
    if visualize:
        # 筛选内点匹配
        inlier_matches = [m for i, m in enumerate(good_matches) if mask[i]]
        
        # 使用OpenCV的drawMatches绘制
        match_img = cv2.drawMatches(
            crop_a, kp_a,
            crop_b, kp_b,
            inlier_matches,
            None,
            matchColor=(0, 255, 0),      # 绿色表示内点
            singlePointColor=(255, 0, 0), # 蓝色表示特征点
            flags=cv2.DrawMatchesFlags_NOT_DRAW_SINGLE_POINTS
        )
        
        # 添加文字信息
        cv2.putText(match_img, f"Total Matches: {len(good_matches)}", 
                    (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 255, 255), 2)
        cv2.putText(match_img, f"Inliers: {len(inlier_matches)}", 
                    (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)
        
        if output_path:
            cv2.imwrite(output_path, match_img)
            print(f"匹配可视化已保存到: {output_path}")
        
        # 显示结果
        cv2.imshow("Feature Matches", match_img)
        cv2.waitKey(0)
        cv2.destroyAllWindows()
    
    return transformation_matrix


def transform_point(point: Tuple[float, float], matrix: np.ndarray) -> Tuple[float, float]:
    """
    使用变换矩阵将点从图像A变换到图像B
    
    Args:
        point: 图像A中的点坐标 (x, y)
        matrix: 3x3变换矩阵
    
    Returns:
        图像B中的对应点坐标 (x, y)
    """
    pt = np.array([[point[0], point[1], 1.0]], dtype=np.float32).T
    transformed = matrix @ pt
    transformed /= transformed[2]
    return (transformed[0, 0], transformed[1, 0])


# 使用示例
if __name__ == "__main__":
    # 读取图像
    path = "tools\\ImageRegistration\\"
    img_a = cv2.imread(path + "a.png")
    img_b = cv2.imread(path + "c.png")
    
    # 定义ROI区域 (x, y, width, height)
    roi_a = (140, 80, 910, 500)
    roi_b = (140, 80, 910, 500)
    
    # 进行配准并可视化
    H = register_images(
        img_a, img_b, roi_a, roi_b, 
        method='SIFT',
        visualize=True,
        output_path=path + "matches.png"
    )
    
    if H is not None:
        print("变换矩阵:")
        print(H)
        
        # 测试点变换
        point_a = (200, 200)
        point_b = transform_point(point_a, H)
        print(f"点 {point_a} 在图像B中的位置: {point_b}")
        
        # 可选: 可视化配准结果
        height, width = img_a.shape[:2]
        warped = cv2.warpPerspective(img_a, H, (img_b.shape[1], img_b.shape[0]))
        cv2.imwrite(path + "warped_result_a.png", warped)
        
        # 可选: 可视化配准结果
        height, width = img_b.shape[:2]
        warped = cv2.warpPerspective(img_b, np.linalg.inv(H), (img_a.shape[1], img_a.shape[0]))
        cv2.imwrite(path + "warped_result_b.png", warped)