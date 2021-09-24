#include "FreeTimeline.h"
#include "Engine/TimelineTemplate.h"
#include "Components/TimelineComponent.h"
#include "Curves/CurveFloat.h"

UFreeTimeline::UFreeTimeline()
{
	PrimaryComponentTick.bCanEverTick = true;
}
// Called when the game starts
void UFreeTimeline::BeginPlay()
{
	Super::BeginPlay();
}
void UFreeTimeline::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	InTimeline.TickTimeline(DeltaTime);
}


void UFreeTimeline::CreateEventTrack(UCurveFloat * EventCurve, FOnTimelineEvent EventDelegate, bool& IsSuccess)
{
	//UTimelineComponent* timeline = GetTimeline();
	if (EventCurve != NULL)
	{
		// 为该轨道中的所有关键帧创建委托
		for (auto It(EventCurve->FloatCurve.GetKeyIterator()); It; ++It)
		{
			//InTimeline->AddEvent(It->Time, EventDelegate);
			InTimeline.AddEvent(It->Time, EventDelegate);
		}
	}
	IsSuccess = true;
}

void UFreeTimeline::PlayFromStart()
{
	Activate();
	InTimeline.PlayFromStart();
}

void UFreeTimeline::Activate(bool bReset)
{
	Super::Activate(bReset);
	PrimaryComponentTick.SetTickFunctionEnable(true);
}
