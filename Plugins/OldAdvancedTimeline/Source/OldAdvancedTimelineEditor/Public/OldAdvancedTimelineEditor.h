// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class OLDADVANCEDTIMELINEEDITOR_API FAdvancedTimelineEditorModule : public IModuleInterface {
public:
	virtual void StartupModule() override;
};