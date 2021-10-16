// Fill out your copyright notice in the Description page of Project Settings.

#include "OldAdvancedTimelineEditor.h"

#include "EdGraphUtilities.h"
#include "Modules/ModuleManager.h"
#include "K2_AdvancedTimelinePinFactory.h"


void FAdvancedTimelineEditorModule::StartupModule()
{
	//把自定义的PinFactory添加到编译网络
	TSharedPtr<FK2AdvancedTimelinePinFactory> K2PinFactory = MakeShareable(new FK2AdvancedTimelinePinFactory());
	FEdGraphUtilities::RegisterVisualPinFactory(K2PinFactory);
}

IMPLEMENT_MODULE(FAdvancedTimelineEditorModule, OldAdvancedTimelineEditor);