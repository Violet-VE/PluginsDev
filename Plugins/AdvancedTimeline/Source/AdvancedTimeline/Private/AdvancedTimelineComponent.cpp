// Fill out your copyright notice in the Description page of Project Settings.

#include "AdvancedTimelineComponent.h"
#include "GameFramework/WorldSettings.h"
#include "Engine/World.h"
#include "Curves/CurveLinearColor.h"
#include "Curves/CurveVector.h"
#include "UObject/Package.h"


DEFINE_LOG_CATEGORY_STATIC(LogAdvTimeline, Log, All);

UAdvancedTimelineComponent::UAdvancedTimelineComponent(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	LengthMode = TL_LastKeyFrame;
	bLooping = false;
	bReversePlayback=false;
	bPlaying = false;
	Length = 5.f;
	PlayRate = 1.f;
	Position = 0.0f;
}

void UAdvancedTimelineComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIgnoreTimeDilation)
	{
		AActor* const OwnerActor = GetOwner();
		if (OwnerActor)
		{
			DeltaTime /= OwnerActor->GetActorTimeDilation();
		}
		else
		{
			// 由于某种原因，没有Actor，使用世界时间膨胀作为后备手段。
			UWorld* const W = GetWorld();
			if (W)
			{
				DeltaTime /= W->GetWorldSettings()->GetEffectiveTimeDilation();
			}
		}
	}
	TickAdvTimeline(DeltaTime);

	if (!IsNetSimulating())
	{
		// Do not deactivate if we are done, since bActive is a replicated property and we shouldn't have simulating
		// clients touch replicated variables.
		if (!bPlaying)
		{
			Deactivate();
		}
	}
}

void UAdvancedTimelineComponent::Play(bool bIsFromStart)
{
	Activate();
	if (bIsFromStart)
		SetPlaybackPosition(0.f);

	bReversePlayback = false;
	bPlaying = true;
}

void UAdvancedTimelineComponent::Reverse(bool bIsFromEnd)
{
	Activate();
	if (bIsFromEnd)
		SetPlaybackPosition(GetTimelineLength());

	bReversePlayback = true;
	bPlaying = true;
}

void UAdvancedTimelineComponent::Pause()
{
	bPlaying = false;
}

void UAdvancedTimelineComponent::Reset()
{
	bPlaying = false;
	SetPlaybackPosition(0.0f,false);
}

bool UAdvancedTimelineComponent::IsPlaying() const
{
	return bPlaying;
}

bool UAdvancedTimelineComponent::IsReversing() const
{
	return bReversePlayback;
}

void UAdvancedTimelineComponent::SetPlaybackPosition(float NewPosition, bool bFireEvents, bool bFireUpdate,bool bIsPlay)
{
	const float OldPosition = Position;
	Position = NewPosition;

	// Iterate over each vector interpolation
	for (const FAdvVectorTrackInfo& CurrentEntry : AdvVectorTracks)
	{
		if (CurrentEntry.VectorCurve && CurrentEntry.OnEventFunc.IsBound())
		{
			// Get vector from curve
			FVector const Vec = CurrentEntry.VectorCurve->GetVectorValue(GetPlaybackPosition());

			// Pass vec to specified function
			CurrentEntry.OnEventFunc.ExecuteIfBound(Vec);
		}
	}

	// Iterate over each float interpolation
	for (const FAdvFloatTrackInfo& CurrentEntry : AdvFloatTracks)
	{
		if (CurrentEntry.FloatCurve && CurrentEntry.OnEventFunc.IsBound())
		{
			// Get float from func
			const float Val = CurrentEntry.FloatCurve->GetFloatValue(GetPlaybackPosition());

			// Pass float to specified function
			CurrentEntry.OnEventFunc.ExecuteIfBound(Val);
		}
	}

	// Iterate over each color interpolation
	for (const FAdvLinearColorTrackInfo& CurrentEntry : AdvLinearColorTracks)
	{
		if (CurrentEntry.LinearColorCurve && CurrentEntry.OnEventFunc.IsBound())
		{
			// Get vector from curve
			const FLinearColor Color = CurrentEntry.LinearColorCurve->GetLinearColorValue(GetPlaybackPosition());

			// Pass vec to specified function
			CurrentEntry.OnEventFunc.ExecuteIfBound(Color);
		}
	}


	// If we should be firing events for this track...
	if (bFireEvents)
	{
		// If playing sequence forwards.
		float MinTime, MaxTime;
		if (!bReversePlayback)
		{
			MinTime = OldPosition;
			MaxTime = Position;

			// Slight hack here.. if playing forwards and reaching the end of the sequence, force it over a little to ensure we fire events actually on the end of the sequence.
			if (MaxTime == GetTimelineLength())
			{
				MaxTime += (float)KINDA_SMALL_NUMBER;
			}
		}
		// If playing sequence backwards.
		else
		{
			MinTime = Position;
			MaxTime = OldPosition;

			// Same small hack as above for backwards case.
			if (MinTime == 0.f)
			{
				MinTime -= (float)KINDA_SMALL_NUMBER;
			}
		}

		// See which events fall into traversed region.
		for (auto ThisEventTrack : AdvEventTracks)
		{
			for (auto It(ThisEventTrack.EventCurve->FloatCurve.GetKeyIterator());It;++It)
			{
				float EventTime = It->Time;

				// Need to be slightly careful here and make behavior for firing events symmetric when playing forwards of backwards.
				bool bFireThisEvent = false;
				if (!bReversePlayback)
				{
					if (EventTime >= MinTime && EventTime < MaxTime)
					{
						bFireThisEvent = true;
					}
				}
				else
				{
					if (EventTime > MinTime && EventTime <= MaxTime)
					{
						bFireThisEvent = true;
					}
				}

				if (bFireThisEvent)
				{
					ThisEventTrack.OnEventFunc.ExecuteIfBound();
				}
			}
		}
	}
	if (bFireUpdate)
	{
		TimelineUpdateFunc.ExecuteIfBound();
	}
	if (bIsPlay)
	{
		Play();
	}
}

float UAdvancedTimelineComponent::GetPlaybackPosition() const
{
	return Position;
}

void UAdvancedTimelineComponent::SetLooping(bool bNewLooping)
{
	bLooping = bNewLooping;
}

bool UAdvancedTimelineComponent::IsLooping() const
{
	return bLooping;
}

void UAdvancedTimelineComponent::SetPlayRate(float NewRate)
{
	PlayRate = NewRate;
}

float UAdvancedTimelineComponent::GetPlayRate() const
{
	return PlayRate;
}

float UAdvancedTimelineComponent::GetTimelineLength() const
{
	switch (LengthMode)
	{
	case TL_TimelineLength:
		return Length;
	case TL_LastKeyFrame:
		return GetTrackLastKeyframeTime();
	default:
		UE_LOG(LogAdvTimeline, Error, TEXT("Invalid timeline length mode on timeline!"));
		return 0.f;
	}
}

void UAdvancedTimelineComponent::SetTimelineLength(float NewLength)
{
	Length = NewLength;
}

void UAdvancedTimelineComponent::SetTimelineLengthMode(ETimelineLengthMode NewLengthMode)
{
	LengthMode = NewLengthMode;
}

void UAdvancedTimelineComponent::SetIgnoreTimeDilation(bool bNewIgnoreTimeDilation)
{
	bIgnoreTimeDilation = bNewIgnoreTimeDilation;
}

bool UAdvancedTimelineComponent::GetIgnoreTimeDilation() const
{
	return bIgnoreTimeDilation;
}

void UAdvancedTimelineComponent::ChangeFloatTrackCurve(UCurveFloat* NewFloatCurve, FName FloatTrackName)
{
	bool bFoundTrack = false;
	if (FloatTrackName != NAME_None && AdvFloatTracks.Num() > 0)
	{
		for (FAdvFloatTrackInfo& CurrentTrack : AdvFloatTracks)
		{
			if (CurrentTrack.TrackName == FloatTrackName)
			{
				CurrentTrack.FloatCurve = NewFloatCurve;
				bFoundTrack = true;
				break;
			}
		}
	}

	if (!bFoundTrack)
	{
		UE_LOG(LogAdvTimeline, Log, TEXT("SetFloatCurve: No float track with name %s!"), *FloatTrackName.ToString());
	}
}

void UAdvancedTimelineComponent::ChangeVectorTrackCurve(UCurveVector* NewVectorCurve, FName VectorTrackName)
{
	bool bFoundTrack = false;
	if (VectorTrackName != NAME_None && AdvVectorTracks.Num() > 0)
	{
		for (FAdvVectorTrackInfo& CurrentTrack : AdvVectorTracks)
		{
			if (CurrentTrack.TrackName == VectorTrackName)
			{
				CurrentTrack.VectorCurve = NewVectorCurve;
				bFoundTrack = true;
				break;
			}
		}
	}

	if (!bFoundTrack)
	{
		UE_LOG(LogAdvTimeline, Log, TEXT("SetFloatCurve: No vector track with name %s!"), *VectorTrackName.ToString());
	}
}

void UAdvancedTimelineComponent::ChangeLinearColorTrackCurve(UCurveLinearColor* NewLinearColorCurve,FName LinearColorTrackName)
{
	bool bFoundTrack = false;
	if (LinearColorTrackName != NAME_None && AdvLinearColorTracks.Num() > 0)
	{
		for (FAdvLinearColorTrackInfo& CurrentTrack : AdvLinearColorTracks)
		{
			if (CurrentTrack.TrackName == LinearColorTrackName)
			{
				CurrentTrack .LinearColorCurve = NewLinearColorCurve;
				bFoundTrack = true;
				break;
			}
		}
	}

	if (!bFoundTrack)
	{
		UE_LOG(LogAdvTimeline, Log, TEXT("SetFloatCurve: No linearcolor track with name %s!"), *LinearColorTrackName.ToString());
	}
}

void UAdvancedTimelineComponent::ChangeEventTrackCurve(UCurveFloat* NewFloatCurve,const FName& EventTrackName)
{
	bool bFoundTrack = false;
	if (EventTrackName != NAME_None && AdvEventTracks.Num()>0)
	{
		for (FAdvEventTrackInfo& CurrentTrack : AdvEventTracks)
		{
			if (CurrentTrack.TrackName == EventTrackName)
			{
				CurrentTrack.EventCurve = NewFloatCurve;
				bFoundTrack = true;
				break;
			}
		}
	}

	if (!bFoundTrack)
	{
		UE_LOG(LogAdvTimeline, Log, TEXT("SetFloatCurve: No event track with name %s!"), *EventTrackName.ToString());
	}
}

void UAdvancedTimelineComponent::Activate(bool bReset)
{
	Super::Activate(bReset);
	PrimaryComponentTick.SetTickFunctionEnable(true);
}

void UAdvancedTimelineComponent::Deactivate()
{
	Super::Deactivate();
	PrimaryComponentTick.SetTickFunctionEnable(false);
}

bool UAdvancedTimelineComponent::IsReadyForOwnerToAutoDestroy() const
{
	return !IsPlaying();
}

UFunction* UAdvancedTimelineComponent::GetTimelineEventSignature()
{
	auto* TimelineEventSig = FindObject<UFunction>(FindPackage(nullptr, TEXT("/Script/Engine")), TEXT("OnTimelineEvent__DelegateSignature"));
	check(TimelineEventSig != NULL)
	return TimelineEventSig;
}

UFunction* UAdvancedTimelineComponent::GetTimelineFloatSignature()
{
	auto* TimelineFloatSig = FindObject<UFunction>(FindPackage(nullptr, TEXT("/Script/Engine")), TEXT("OnTimelineFloat__DelegateSignature"));
	check(TimelineFloatSig != NULL)
	return TimelineFloatSig;
}

UFunction* UAdvancedTimelineComponent::GetTimelineVectorSignature()
{
	auto* TimelineVectorSig = FindObject<UFunction>(FindPackage(nullptr, TEXT("/Script/Engine")), TEXT("OnTimelineVector__DelegateSignature"));
	check(TimelineVectorSig != NULL)
	return TimelineVectorSig;
}

UFunction* UAdvancedTimelineComponent::GetTimelineLinearColorSignature()
{
	auto* TimelineVectorSig = FindObject<UFunction>(FindPackage(nullptr, TEXT("/Script/Engine")), TEXT("OnTimelineLinearColor__DelegateSignature"));
	check(TimelineVectorSig != NULL)
	return TimelineVectorSig;
}

ETimelineSigType UAdvancedTimelineComponent::GetTimelineSignatureForFunction(const UFunction* InFunc)
{
	if (!InFunc)
	{
		if (InFunc->IsSignatureCompatibleWith(GetTimelineEventSignature()))
		{
			return ETS_EventSignature;
		}
		else if (InFunc->IsSignatureCompatibleWith(GetTimelineFloatSignature()))
		{
			return ETS_FloatSignature;
		}
		else if (InFunc->IsSignatureCompatibleWith(GetTimelineVectorSignature()))
		{
			return ETS_VectorSignature;
		}
		else if (InFunc->IsSignatureCompatibleWith(GetTimelineLinearColorSignature()))
		{
			return ETS_LinearColorSignature;
		}
	}

	return ETS_InvalidSignature;
}

void UAdvancedTimelineComponent::AddEventTrack(FAdvEventTrackInfo InEventTrack,bool& bIsSuccess)
{
	AdvEventTracks.Add(InEventTrack);
}

void UAdvancedTimelineComponent::AddVectorTrack(FAdvVectorTrackInfo InVectorTrack, bool& bIsSuccess)
{
	AdvVectorTracks.Add(InVectorTrack);
}

void UAdvancedTimelineComponent::AddFloatTrack(FAdvFloatTrackInfo InFloatTrack, bool& bIsSuccess)
{
	AdvFloatTracks.Add(InFloatTrack);
}

void UAdvancedTimelineComponent::AddLinearColorTrack(FAdvLinearColorTrackInfo InLinearColorTrack, bool& bIsSuccess)
{
	AdvLinearColorTracks.Add(InLinearColorTrack);
}

void UAdvancedTimelineComponent::SetUpdateEvent(FOnTimelineEvent NewTimelinePostUpdateFunc)
{
	TimelinePostUpdateFunc = NewTimelinePostUpdateFunc;
}

void UAdvancedTimelineComponent::SetFinishedEvent(const FOnTimelineEvent& NewTimelineFinishedFunc)
{
	TimelineFinishedFunc = NewTimelineFinishedFunc;
}

void UAdvancedTimelineComponent::TickAdvTimeline(float DeltaTime)
{
	bool bIsFinished = false;

	if (bPlaying)
	{
		const float TimelineLength = GetTimelineLength();
		const float EffectiveDeltaTime = DeltaTime * (bReversePlayback ? (-PlayRate) : (PlayRate));

		float NewPosition = Position + EffectiveDeltaTime;

		if (EffectiveDeltaTime > 0.0f)
		{
			if (NewPosition > TimelineLength)
			{
				// If looping, play to end, jump to start, and set target to somewhere near the beginning.
				if (bLooping)
				{
					SetPlaybackPosition(TimelineLength, true);
					SetPlaybackPosition(0.f, false);

					if (TimelineLength > 0.f)
					{
						while (NewPosition > TimelineLength)
						{
							NewPosition -= TimelineLength;
						}
					}
					else
					{
						NewPosition = 0.f;
					}
				}
				// If not looping, snap to end and stop playing.
				else
				{
					NewPosition = TimelineLength;
					Reset();
					bIsFinished = true;
				}
			}
		}
		else
		{
			if (NewPosition < 0.f)
			{
				// If looping, play to start, jump to end, and set target to somewhere near the end.
				if (bLooping)
				{
					SetPlaybackPosition(0.f, true);
					SetPlaybackPosition(TimelineLength, false);

					if (TimelineLength > 0.f)
					{
						while (NewPosition < 0.f)
						{
							NewPosition += TimelineLength;
						}
					}
					else
					{
						NewPosition = 0.f;
					}
				}
				// If not looping, snap to start and stop playing.
				else
				{
					NewPosition = 0.f;
					Reset();
					bIsFinished = true;
				}
			}
		}

		SetPlaybackPosition(NewPosition, true);
	}

	// Notify user that timeline finished
	if (bIsFinished)
		TimelineFinishedFunc.ExecuteIfBound();
}

void UAdvancedTimelineComponent::GetAllTrackData(TArray<UCurveBase*>& OutCurves,TArray<FName>& OutTrackName,TArray<FName>& OutFuncName) const
{
	for (const FAdvEventTrackInfo& ThisTrack : AdvEventTracks)
	{
		OutCurves.Add(ThisTrack.EventCurve);
		OutFuncName.Add(ThisTrack.OnEventFunc.GetFunctionName());
		OutTrackName.Add(ThisTrack.TrackName);
	}

	for (const FAdvFloatTrackInfo& ThisTrack : AdvFloatTracks)
	{
		OutCurves.Add(ThisTrack.FloatCurve);
		OutFuncName.Add(ThisTrack.OnEventFunc.GetFunctionName());
		OutTrackName.Add(ThisTrack.TrackName);
	}

	for (const FAdvVectorTrackInfo& ThisTrack : AdvVectorTracks)
	{
		OutCurves.Add(ThisTrack.VectorCurve);
		OutFuncName.Add(ThisTrack.OnEventFunc.GetFunctionName());
		OutTrackName.Add(ThisTrack.TrackName);
	}

	for (const FAdvLinearColorTrackInfo& ThisTrack : AdvLinearColorTracks)
	{
		OutCurves.Add(ThisTrack.LinearColorCurve);
		OutFuncName.Add(ThisTrack.OnEventFunc.GetFunctionName());
		OutTrackName.Add(ThisTrack.TrackName);
	}
}

float UAdvancedTimelineComponent::GetTrackLastKeyframeTime() const
{

	float MaxTime = 0.f;

	// Check the various tracks for the max time specified
	for (const FAdvEventTrackInfo& CurrentEvent : AdvEventTracks)
	{
		float MinVal, MaxVal;
		CurrentEvent.EventCurve->GetTimeRange(MinVal, MaxVal);
		MaxTime = FMath::Max(MaxVal, MaxTime);
	}

	for (const FAdvVectorTrackInfo& CurrentTrack : AdvVectorTracks)
	{
		float MinVal, MaxVal;
		CurrentTrack.VectorCurve->GetTimeRange(MinVal, MaxVal);
		MaxTime = FMath::Max(MaxVal, MaxTime);
	}

	for (const FAdvFloatTrackInfo& CurrentTrack : AdvFloatTracks)
	{
		float MinVal, MaxVal;
		CurrentTrack.FloatCurve->GetTimeRange(MinVal, MaxVal);
		MaxTime = FMath::Max(MaxVal, MaxTime);
	}

	for (const FAdvLinearColorTrackInfo& CurrentTrack : AdvLinearColorTracks)
	{
		float MinVal, MaxVal;
		CurrentTrack.LinearColorCurve->GetTimeRange(MinVal, MaxVal);
		MaxTime = FMath::Max(MaxVal, MaxTime);
	}

	return MaxTime;
}

float UAdvancedTimelineComponent::GetCurveLastKeyframeTime(ETrackType InTrackType, UCurveBase* InCurve) const
{

	float MaxTime = 0.f;

	// 思路错了，等待重写
	//switch (InTrackType)
	//{
	//	case ETrackType::EventTrack:
	//		if (UCurveFloat* CurrentCurve = Cast<UCurveFloat>(InCurve))
	//		{
	//			float MinVal, MaxVal;
	//			CurrentCurve->GetTimeRange(MinVal, MaxVal);
	//			MaxTime = FMath::Max(MaxVal, MaxTime);
	//		}
	//		break;
	//	case ETrackType::FloatTrack:
	//		if (UCurveFloat* CurrentCurve = Cast<UCurveFloat>(InCurve))
	//		{
	//			float MinVal, MaxVal;
	//			CurrentCurve->GetTimeRange(MinVal, MaxVal);
	//			MaxTime = FMath::Max(MaxVal, MaxTime);
	//		}
	//		break;
	//	case ETrackType::LinearColorTrack:
	//		if (UCurveLinearColor* CurrentCurve = Cast<UCurveLinearColor>(InCurve))
	//		{
	//			float MinVal, MaxVal;
	//			CurrentCurve->GetTimeRange(MinVal, MaxVal);
	//			MaxTime = FMath::Max(MaxVal, MaxTime);
	//		}
	//		break;
	//	case ETrackType::VectorTrack:
	//		if (UCurveVector* CurrentCurve = Cast<UCurveVector>(InCurve))
	//		{
	//			float MinVal, MaxVal;
	//			CurrentCurve->GetTimeRange(MinVal, MaxVal);
	//			MaxTime = FMath::Max(MaxVal, MaxTime);
	//		}
	//		break;
	//	default:
	//		break;
	//}
	return MaxTime;
}

