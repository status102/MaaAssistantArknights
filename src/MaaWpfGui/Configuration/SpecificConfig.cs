// <copyright file="SpecificConfig.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System.Text.Json.Serialization;
using ObservableCollections;

namespace MaaWpfGui.Configuration
{
    public class SpecificConfig
    {
        // ReSharper disable once AutoPropertyCanBeMadeGetOnly.Global
        [JsonInclude] public GUI GUI { get; private set; } = new();

        [JsonInclude] public ObservableDictionary<string, int> InfrastOrder { get; private set; } = new();

        [JsonInclude] public ObservableList<BaseTask> TaskQueue { get; private set; } = new();

        [JsonInclude] public ObservableDictionary<string, bool> DragItemIsChecked { get; private set; } = new();

        public class StartUpTask : BaseTask
        {
        }

        public class CloseDownTask : BaseTask
        {
        }

        public class FightTask : BaseTask
        {
        }

        public class AwardTask : BaseTask
        {
        }

        public class MallTask : BaseTask
        {
        }

        public class InfrastTask : BaseTask
        {
        }

        public class RecruitTask : BaseTask
        {
        }

        public class RoguelikeTask : BaseTask
        {
        }

        public class CopilotTask : BaseTask
        {
        }

        public class SSSCopilotTask : BaseTask
        {
        }

        public class SingleStepTask : BaseTask
        {
        }

        public class VideoRecognition : BaseTask
        {
        }

        public class DepotTask : BaseTask
        {
        }

        public class OperBoxTask : BaseTask
        {
        }

        public class ReclamationTask : BaseTask
        {
        }

        public class CustomTask : BaseTask
        {
        }

        [JsonDerivedType(typeof(StartUpTask), typeDiscriminator: "StartUpTask")]
        [JsonDerivedType(typeof(CloseDownTask), typeDiscriminator: "CloseDownTask")]
        [JsonDerivedType(typeof(FightTask), typeDiscriminator: "FightTask")]
        [JsonDerivedType(typeof(AwardTask), typeDiscriminator: "AwardTask")]
        [JsonDerivedType(typeof(MallTask), typeDiscriminator: "MallTask")]
        [JsonDerivedType(typeof(InfrastTask), typeDiscriminator: "InfrastTask")]
        [JsonDerivedType(typeof(RecruitTask), typeDiscriminator: "RecruitTask")]
        [JsonDerivedType(typeof(RoguelikeTask), typeDiscriminator: "RoguelikeTask")]
        [JsonDerivedType(typeof(CopilotTask), typeDiscriminator: "CopilotTask")]
        [JsonDerivedType(typeof(SSSCopilotTask), typeDiscriminator: "SSSCopilotTask")]
        [JsonDerivedType(typeof(SingleStepTask), typeDiscriminator: "SingleStepTask")]
        [JsonDerivedType(typeof(VideoRecognition), typeDiscriminator: "VideoRecognition")]
        [JsonDerivedType(typeof(DepotTask), typeDiscriminator: "DepotTask")]
        [JsonDerivedType(typeof(OperBoxTask), typeDiscriminator: "OperBoxTask")]
        [JsonDerivedType(typeof(ReclamationTask), typeDiscriminator: "ReclamationTask")]
        [JsonDerivedType(typeof(CustomTask), typeDiscriminator: "CustomTask")]
        public class BaseTask
        {
            public string Name { get; set; } = string.Empty;

            [JsonInclude]
            public bool IsChecked { get; set; } = false;

            [JsonInclude]
            public TaskTypeEnum TaskType { get; set; }
        }

        public enum TaskTypeEnum
        {
            /// <summary>
            /// 开始唤醒。
            /// </summary>
            StartUp = 0,

            CloseDown,

            /// <summary>
            /// 刷理智。
            /// </summary>
            Fight,
            Award,
            Mall,
            Infrast,
            Recruit,
            Roguelike,
            Copilot,
            SSSCopilot,
            SingleStep,
            VideoRecognition,
            Depot,
            OperBox,
            Reclamation,
            Custom,
        }
    }
}
