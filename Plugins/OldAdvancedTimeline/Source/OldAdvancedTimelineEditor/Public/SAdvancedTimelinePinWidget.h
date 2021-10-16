// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "SGraphPinNameList.h"


class OLDADVANCEDTIMELINEEDITOR_API SAdvancedTimelinePinWidget : public SGraphPinNameList {
public:
	SLATE_BEGIN_ARGS(SAdvancedTimelinePinWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj, const TArray<FString>& InNameList);


protected:

	void RefreshNameList(const TArray<FString>& InNameList);
};