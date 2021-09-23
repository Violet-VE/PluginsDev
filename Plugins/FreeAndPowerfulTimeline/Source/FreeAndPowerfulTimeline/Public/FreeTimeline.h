// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/TimelineComponent.h"
#include "FreeTimeline.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FREEANDPOWERFULTIMELINE_API UFreeTimeline : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UFreeTimeline();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
private:
	UTimelineComponent InTimeline;
};
