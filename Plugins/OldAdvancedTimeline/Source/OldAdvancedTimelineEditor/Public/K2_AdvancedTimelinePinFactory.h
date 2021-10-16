// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"


class OLDADVANCEDTIMELINEEDITOR_API FK2AdvancedTimelinePinFactory : public FGraphPanelPinFactory {
public:
	virtual TSharedPtr<SGraphPin> CreatePin(class UEdGraphPin* InPin) const override;
};