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

protected:
	// Called when the game starts
	virtual void BeginPlay() override;


private:

	UPROPERTY()
		FTimeline InTimeline;
	virtual void Activate(bool bReset = false) override;


public:	
	UFreeTimeline();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	UFUNCTION(BlueprintCallable, meta = (ToolTip = "创建时间线的事件轨道。可以直接绑定一个事件上去。"), Category = "FreeTimeline|Create")
		void CreateEventTrack(UCurveFloat* EventCurve, FOnTimelineEvent EventDelegate, bool& IsSuccess);
	
	UFUNCTION(BlueprintCallable, meta = (ToolTip = "获得当前时间"), Category = "FreeTimeline|Get")
		void PlayFromStart();


};
