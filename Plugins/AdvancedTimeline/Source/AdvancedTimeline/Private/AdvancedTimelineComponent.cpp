// Fill out your copyright notice in the Description page of Project Settings.

// ReSharper disable CppExpressionWithoutSideEffects
#include "AdvancedTimelineComponent.h"

#include "Culture.h"
#include "EdGraphCompilerUtilities.h"
#include "Kismet2/CompilerResultsLog.h"
#include "GameFramework/WorldSettings.h"
#include "Engine/World.h"
#include "Curves/CurveLinearColor.h"
#include "Curves/CurveVector.h"
#include "Engine/Engine.h"
#include "UObject/Package.h"
#include "Kismet/KismetGuidLibrary.h"

//本地化
#define LOCTEXT_NAMESPACE "AdvTimeline"
/** 本地化指南：
 *	只需要使用NSLOCTEXT()或者LOCTEXT()就可以了，剩下的操作需要进编辑器打开本地化Dash面板再改
 */

UAdvancedTimelineComponent::UAdvancedTimelineComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

// Finished
void UAdvancedTimelineComponent::Activate(bool bReset)
{
	Super::Activate(bReset);
	PrimaryComponentTick.SetTickFunctionEnable(true);
}

// Finished
void UAdvancedTimelineComponent::Deactivate()
{
	Super::Deactivate();
	PrimaryComponentTick.SetTickFunctionEnable(false);
}

// Finished
bool UAdvancedTimelineComponent::IsReadyForOwnerToAutoDestroy() const
{
	return !PrimaryComponentTick.IsTickFunctionEnabled();
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::TickComponent(float DeltaTime, ELevelTick Tick,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, Tick, ThisTickFunction);

	if (Timelines.Num() > 0)
	{
		UE_LOG(LogAdvTimeline, Warning, TEXT("Ticking"));
		for (TPair<FString, FTimelineInfo> ThisTimeline : Timelines)
		{
			if (!ValidFTimelineInfo(&ThisTimeline.Value)) continue;
			FTimelineSetting* ThisSetting;
			if (!QuerySetting(ThisTimeline.Value.SettingGuName, ThisSetting) && !ValidFTimelineSetting(ThisSetting)) continue;
			if (ThisSetting->bIgnoreTimeDilation)
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
			TickTimeline(ThisTimeline.Value.GuName,DeltaTime);

			if (!IsNetSimulating())
			{
				// Do not deactivate if we are done, since bActive is a replicated property and we shouldn't have simulating
				// clients touch replicated variables.
				if (!ThisTimeline.Value.IsPlaying)
				{
					Deactivate();
				}
			}
		}
	}
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::TickTimeline(const FString InTimelineGuName, float DeltaTime)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}

	FTimelineInfo* ThisTimeline;
	QueryTimeline(InTimelineGuName, ThisTimeline);

	if (ValidFTimelineInfo(ThisTimeline))
	{
		FTimelineSetting* ThisSetting;
		QuerySetting(ThisTimeline->SettingGuName, ThisSetting);
		if (ValidFTimelineSetting(ThisSetting)) {
			bool bIsFinished = false;

			if (ThisTimeline->IsPlaying)
			{
				const float TimelineLength = GetTimelineLength(ThisTimeline->GuName);
				const float EffectiveDeltaTime = DeltaTime * (ThisTimeline->bReversePlayback ? (-ThisSetting->PlayRate) : (ThisSetting->PlayRate));

				float NewPosition = ThisTimeline->Position + EffectiveDeltaTime;

				if (EffectiveDeltaTime > 0.0f)
				{
					if (NewPosition > TimelineLength)
					{
						// If looping, play to end, jump to start, and set target to somewhere near the beginning.
						if (ThisSetting->bIsLoop)
						{
							SetPlaybackPosition(ThisTimeline->GuName, TimelineLength, true);
							SetPlaybackPosition(ThisTimeline->GuName, 0.f, false);

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
							Stop(ThisTimeline->GuName, true);
							bIsFinished = true;
						}
					}
				}
				else
				{
					if (NewPosition < 0.f)
					{
						// If looping, play to start, jump to end, and set target to somewhere near the end.
						if (ThisSetting->bIsLoop)
						{
							SetPlaybackPosition(ThisTimeline->GuName, 0.f, true);
							SetPlaybackPosition(ThisTimeline->GuName, TimelineLength, false);

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
							Stop(ThisTimeline->GuName);
							bIsFinished = true;
						}
					}
				}

				SetPlaybackPosition(ThisTimeline->GuName, NewPosition, true);
			}

			// Notify user that timeline finished
			if (bIsFinished)
				ThisSetting->EventFunc.OnFinishedEventFunc.ExecuteIfBound();
		}
	}
	PrintError(LOCTEXT("TickTimelineError","Tick timeline with errors! Please check if the required parameters are filled in the timeline!"));
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::Play(const FString InTimelineGuName, const EPlayMethod PlayMethod)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}

	FTimelineInfo* ThisTimeline;

	if (QueryTimeline(InTimelineGuName, ThisTimeline))
	{
		FTimelineSetting* ThisSetting;
		if (QuerySetting(ThisTimeline->SettingGuName, ThisSetting)) {
			Activate();
			if (PlayMethod == EPlayMethod::PlayOnStart)
				SetPlaybackPosition(ThisTimeline->GuName, 0.f, true);
			else if (PlayMethod == EPlayMethod::Reverse)
				ThisTimeline->bReversePlayback = true;
			else if (PlayMethod == EPlayMethod::ReverseOnEnd)
			{
				ThisTimeline->bReversePlayback = true;
				SetPlaybackPosition(ThisTimeline->GuName, ThisSetting->Length, true);
			}
			ThisTimeline->IsPlaying = true;
		}
		PrintError("Play");
		return;
	}
	PrintError(LOCTEXT("PlayError",
		"There is an error in the timeline that needs to be played, please check if the required parameters are filled in!"));
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::Stop(const FString InTimelineGuName, const bool bIsPause)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}

	FTimelineInfo* ThisTimeline;

	if (QueryTimeline(InTimelineGuName, ThisTimeline))
	{
		FTimelineSetting* ThisSetting;
		if (QuerySetting(ThisTimeline->SettingGuName, ThisSetting)) {
			if (!bIsPause)
			{
				SetPlaybackPosition(ThisTimeline->GuName, 0.f, true);
			}
			ThisTimeline->IsPlaying = false;
		}
		PrintError("Stop");
		return;
	}
	PrintError("Stop");
}

// Finished TODO:Wait Valid
float UAdvancedTimelineComponent::GetPlaybackPosition(const FString InTimelineGuName)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return 0;
	}

	FTimelineInfo* ThisTimeline;

	if (QueryTimeline(InTimelineGuName, ThisTimeline))
	{
		return ThisTimeline->Position;
	}
	PrintError("GetPlaybackPosition");
	return 0;
}

// Finished TODO:Wait Valid
bool UAdvancedTimelineComponent::GetIgnoreTimeDilation(const FString InTimelineGuName)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return false;
	}

	FTimelineInfo* ThisTimeline;

	if (QueryTimeline(InTimelineGuName, ThisTimeline))
	{
		FTimelineSetting* ThisSetting;
		if (QuerySetting(ThisTimeline->SettingGuName, ThisSetting)) {
			return ThisSetting->bIgnoreTimeDilation;
		}
		PrintError("GetIgnoreTimeDilation");
		return false;
	}
	PrintError("GetIgnoreTimeDilation");
	return false;
}

// Finished TODO:Wait Valid
float UAdvancedTimelineComponent::GetPlayRate(const FString InTimelineGuName)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return false;
	}

	FTimelineInfo* ThisTimeline;

	if (QueryTimeline(InTimelineGuName, ThisTimeline))
	{
		FTimelineSetting* ThisSetting;
		if (QuerySetting(ThisTimeline->SettingGuName, ThisSetting)) {
			return ThisSetting->PlayRate;
		}
		PrintError("GetPlayRate");
		return false;
	}
	PrintError("GetPlayRate");
	return false;
}

// Finished TODO:Wait Valid
ELengthMode UAdvancedTimelineComponent::GetLengthMode(const FString InTimelineGuName)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return ELengthMode::Keyframe;
	}

	FTimelineInfo* ThisTimeline;

	if (QueryTimeline(InTimelineGuName, ThisTimeline))
	{
		FTimelineSetting* ThisSetting;
		if (QuerySetting(ThisTimeline->SettingGuName, ThisSetting)) {
			return ThisSetting->LengthMode;
		}
		PrintError("GetLengthMode");
		return ELengthMode::Keyframe;
	}
	PrintError("GetLengthMode");
	return ELengthMode::Keyframe;
}

// Finished TODO:Wait Valid
bool UAdvancedTimelineComponent::GetIsLoop(const FString InTimelineGuName)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return false;
	}

	FTimelineInfo* ThisTimeline;

	if (QueryTimeline(InTimelineGuName, ThisTimeline))
	{
		FTimelineSetting* ThisSetting;
		if (QuerySetting(ThisTimeline->SettingGuName, ThisSetting)) {
			return ThisSetting->bIsLoop;
		}
	}

	PrintError("GetIsLoop");
	return false;
}

// Finished TODO:Wait Valid
bool UAdvancedTimelineComponent::GetIsPause(const FString InTimelineGuName)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return false;
	}

	FTimelineInfo* ThisTimeline;

	if (QueryTimeline(InTimelineGuName, ThisTimeline))
	{
		FTimelineSetting* ThisSetting;
		if (QuerySetting(ThisTimeline->SettingGuName, ThisSetting)) {
			if (ThisTimeline->IsStop && ThisTimeline->Position > 0 && ThisTimeline->Position < ThisSetting->Length)
				return true;
		}
	}

	PrintError("GetIsPause");
	return false;
}

// Finished TODO:Wait Valid
bool UAdvancedTimelineComponent::GetIsPlaying(const FString InTimelineGuName)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return false;
	}

	FTimelineInfo* ThisTimeline;

	if (QueryTimeline(InTimelineGuName, ThisTimeline))
	{
		return ThisTimeline->IsPlaying;
	}

	PrintError("GetIsPlaying");
	return false;
}

// Finished TODO:Wait Valid
bool UAdvancedTimelineComponent::GetIsStop(const FString InTimelineGuName)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return false;
	}

	FTimelineInfo* ThisTimeline;
	
	if (QueryTimeline(InTimelineGuName, ThisTimeline))
	{
		return ThisTimeline->IsStop;
	}

	PrintError("GetIsStop");
	return false;
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::GetAllFloatTracksInfo(const FString InTimelineGuName,
	TArray<FFloatTrackInfo>& OutFloatTracks)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}

	FTimelineInfo* ThisTimeline;
	QueryTimeline(InTimelineGuName, ThisTimeline);

	if (ValidFTimelineInfo(ThisTimeline))
	{
		if (ThisTimeline->FloatTracks.Num() > 0)
		{
			TArray<FFloatTrackInfo> FloatTrack;
			for (TPair < FString, FFloatTrackInfo> ThisTrack : ThisTimeline->FloatTracks)
			{
				if (ValidFTrackInfo(&ThisTrack.Value)) FloatTrack.Add(ThisTrack.Value);
			}
			OutFloatTracks = FloatTrack;
			return;
		}
		PrintError(LOCTEXT("GetAllFloatTracksInfoError", "Number of FloatTracks is 0"));
		return;
	}
	PrintError("GetAllFloatTracksInfo");
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::GetAllEventTracksInfo(const FString InTimelineGuName,
	TArray<FEventTrackInfo>& OutEventTracks)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}

	FTimelineInfo* ThisTimeline;
	QueryTimeline(InTimelineGuName, ThisTimeline);

	if (ValidFTimelineInfo(ThisTimeline))
	{
		if (ThisTimeline->EventTracks.Num() > 0)
		{
			TArray<FEventTrackInfo> EventTrack;
			for (TPair < FString, FEventTrackInfo> ThisTrack : ThisTimeline->EventTracks)
			{
				if (ValidFTrackInfo(&ThisTrack.Value)) EventTrack.Add(ThisTrack.Value);
			}
			OutEventTracks = EventTrack;
			return;
		}
		PrintError(LOCTEXT("GetAllEventTracksInfoError", "Number of EventTracks is 0"));
		return;
	}
	PrintError("GetAllEventTracksInfo");
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::GetAllVectorTracksInfo(const FString InTimelineGuName,
	TArray<FVectorTrackInfo>& OutVectorTracks)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}

	FTimelineInfo* ThisTimeline;
	QueryTimeline(InTimelineGuName, ThisTimeline);

	if (ValidFTimelineInfo(ThisTimeline))
	{
		if (ThisTimeline->VectorTracks.Num() > 0)
		{
			TArray<FVectorTrackInfo> VectorTrack;
			for (TPair < FString, FVectorTrackInfo> ThisTrack : ThisTimeline->VectorTracks)
			{
				if (ValidFTrackInfo(&ThisTrack.Value)) VectorTrack.Add(ThisTrack.Value);
			}
			OutVectorTracks = VectorTrack;
			return;
		}
		PrintError(LOCTEXT("GetAllVectorTracksInfoError", "Number of VectorTracks is 0"));
		return;
	}
	PrintError("GetAllVectorTracksInfo");
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::GetAllLinearColorTracksInfo(const FString InTimelineGuName,
	TArray<FLinearColorTrackInfo>& OutLinearColorTracks)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}

	FTimelineInfo* ThisTimeline;
	QueryTimeline(InTimelineGuName, ThisTimeline);

	if (ValidFTimelineInfo(ThisTimeline))
	{
		if (ThisTimeline->LinearColorTracks.Num() > 0)
		{
			TArray<FLinearColorTrackInfo> LinearColorTrack;
			for (TPair < FString, FLinearColorTrackInfo> ThisTrack : ThisTimeline->LinearColorTracks)
			{
				if (ValidFTrackInfo(&ThisTrack.Value)) LinearColorTrack.Add(ThisTrack.Value);
			}
			OutLinearColorTracks = LinearColorTrack;
			return;
		}
		PrintError(LOCTEXT("GetAllLinearColorTracksInfoError", "Number of LinearColorTracks is 0"));
		return;
	}
	PrintError("GetAllLinearColorTracksInfo");
}

// Finished TODO:Wait Valid
int UAdvancedTimelineComponent::GetTrackCount(const FString InTimelineGuName)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return 0;
	}

	FTimelineInfo* ThisTimeline;
	
	if (QueryTimeline(InTimelineGuName, ThisTimeline))
	{
		return ThisTimeline->EventTracks.Num() +
			ThisTimeline->FloatTracks.Num() +
			ThisTimeline->VectorTracks.Num() +
			ThisTimeline->LinearColorTracks.Num();
	}
	PrintError("GetTrackCount");
	return 0;
}

// Finished TODO:Wait Valid
float UAdvancedTimelineComponent::GetMinKeyframeTime(const FString InTimelineGuName)
{
	float MinKeyTime = 9999999.0f;

	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return MinKeyTime;
	}

	FTimelineInfo* ThisTimeline;
	
	if (QueryTimeline(InTimelineGuName, ThisTimeline))
	{
		#pragma region
		if (ThisTimeline->EventTracks.Num() > 0)
		{
			for (TPair < FString, FEventTrackInfo> ThisTrack : ThisTimeline->EventTracks)
			{
				if (ValidFTrackInfo(&ThisTrack.Value) && ThisTrack.Value.CurveInfo.EventKey.Num() > 0)
				{
					if (ThisTrack.Value.CurveInfo.CurveObj->FloatCurve.GetFirstKey().Time < MinKeyTime)
						MinKeyTime = ThisTrack.Value.CurveInfo.CurveObj->FloatCurve.GetFirstKey().Time;
				}
			}
		}
		#pragma endregion EventTrack

		#pragma region
		if (ThisTimeline->FloatTracks.Num() > 0)
		{
			for (TPair < FString, FFloatTrackInfo> ThisTrack : ThisTimeline->FloatTracks)
			{
				if (ValidFTrackInfo(&ThisTrack.Value) && ThisTrack.Value.CurveInfo.CurveKeyInfo.Num() > 0)
				{
					if (ThisTrack.Value.CurveInfo.CurveObj->FloatCurve.GetFirstKey().Time < MinKeyTime)
						MinKeyTime = ThisTrack.Value.CurveInfo.CurveObj->FloatCurve.GetFirstKey().Time;
				}
			}
		}
		#pragma endregion FloatTrack

		#pragma region
		if (ThisTimeline->VectorTracks.Num() > 0)
		{
			for (TPair < FString, FVectorTrackInfo> ThisTrack : ThisTimeline->VectorTracks)
			{
				if (ValidFTrackInfo(&ThisTrack.Value)) {
					if (ThisTrack.Value.CurveInfo.XKeyInfo.Num() > 0)
					{
						if (ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[0].GetFirstKey().Time < MinKeyTime)
							MinKeyTime = ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[0].GetFirstKey().Time;
					}
					if (ThisTrack.Value.CurveInfo.YKeyInfo.Num() > 0)
					{
						if (ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[1].GetFirstKey().Time < MinKeyTime)
							MinKeyTime = ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[1].GetFirstKey().Time;
					}
					if (ThisTrack.Value.CurveInfo.ZKeyInfo.Num() > 0)
					{
						if (ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[2].GetFirstKey().Time < MinKeyTime)
							MinKeyTime = ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[2].GetFirstKey().Time;
					}
				}
			}
		}
		#pragma endregion VectorTrack

		#pragma region
		if (ThisTimeline->LinearColorTracks.Num() > 0)
		{
			for (TPair < FString, FLinearColorTrackInfo> ThisTrack : ThisTimeline->LinearColorTracks)
			{
				if (ValidFTrackInfo(&ThisTrack.Value)) {
					if (ThisTrack.Value.CurveInfo.RKeyInfo.Num() > 0)
					{
						if (ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[0].GetFirstKey().Time < MinKeyTime)
							MinKeyTime = ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[0].GetFirstKey().Time;
					}
					if (ThisTrack.Value.CurveInfo.GKeyInfo.Num() > 0)
					{
						if (ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[1].GetFirstKey().Time < MinKeyTime)
							MinKeyTime = ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[1].GetFirstKey().Time;
					}
					if (ThisTrack.Value.CurveInfo.BKeyInfo.Num() > 0)
					{
						if (ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[2].GetFirstKey().Time < MinKeyTime)
							MinKeyTime = ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[2].GetFirstKey().Time;
					}
					if (ThisTrack.Value.CurveInfo.AKeyInfo.Num() > 0)
					{
						if (ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[3].GetFirstKey().Time < MinKeyTime)
							MinKeyTime = ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[3].GetFirstKey().Time;
					}
				}
			}
		}
		#pragma endregion LinearColorTrack
	}
	else PrintError("GetMinKeyframeTime");
	return MinKeyTime;
}

// Finished TODO:Wait Valid
float UAdvancedTimelineComponent::GetMaxKeyframeTime(const FString InTimelineGuName)
{
	float MaxKeyTime = 0.0f;

	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return MaxKeyTime;
	}

	FTimelineInfo* ThisTimeline;
	QueryTimeline(InTimelineGuName, ThisTimeline);

	if (ValidFTimelineInfo(ThisTimeline))
	{
		#pragma region
		if (ThisTimeline->EventTracks.Num() > 0)
		{
			for (TPair<FString,FEventTrackInfo> ThisTrack : ThisTimeline->EventTracks)
			{
				if (ValidFTrackInfo(&ThisTrack.Value) && ThisTrack.Value.CurveInfo.EventKey.Num() > 0)
				{
					if (ThisTrack.Value.CurveInfo.CurveObj->FloatCurve.GetLastKey().Time > MaxKeyTime)
						MaxKeyTime = ThisTrack.Value.CurveInfo.CurveObj->FloatCurve.GetLastKey().Time;
				}
			}
		}
		#pragma endregion EventTrack

		#pragma region
		if (ThisTimeline->FloatTracks.Num() > 0)
		{
			for (TPair < FString, FFloatTrackInfo> ThisTrack : ThisTimeline->FloatTracks)
			{
				if (ValidFTrackInfo(&ThisTrack.Value) && ThisTrack.Value.CurveInfo.CurveKeyInfo.Num() > 0)
				{
					if (ThisTrack.Value.CurveInfo.CurveObj->FloatCurve.GetLastKey().Time > MaxKeyTime)
						MaxKeyTime = ThisTrack.Value.CurveInfo.CurveObj->FloatCurve.GetLastKey().Time;
				}
			}
		}
		#pragma endregion FloatTrack

		#pragma region
		if (ThisTimeline->VectorTracks.Num() > 0)
		{
			for (TPair < FString, FVectorTrackInfo> ThisTrack : ThisTimeline->VectorTracks)
			{
				if (ValidFTrackInfo(&ThisTrack.Value)) {
					if (ThisTrack.Value.CurveInfo.XKeyInfo.Num() > 0)
					{
						if (ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[0].GetLastKey().Time > MaxKeyTime)
							MaxKeyTime = ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[0].GetLastKey().Time;
					}
					if (ThisTrack.Value.CurveInfo.YKeyInfo.Num() > 0)
					{
						if (ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[1].GetLastKey().Time > MaxKeyTime)
							MaxKeyTime = ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[1].GetLastKey().Time;
					}
					if (ThisTrack.Value.CurveInfo.ZKeyInfo.Num() > 0)
					{
						if (ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[2].GetLastKey().Time > MaxKeyTime)
							MaxKeyTime = ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[2].GetLastKey().Time;
					}
				}
			}
		}
		#pragma endregion VectorTrack

		#pragma region
		if (ThisTimeline->LinearColorTracks.Num() > 0)
		{
			for (TPair < FString, FLinearColorTrackInfo> ThisTrack : ThisTimeline->LinearColorTracks)
			{
				if (ValidFTrackInfo(&ThisTrack.Value)) {
					if (ThisTrack.Value.CurveInfo.RKeyInfo.Num() > 0)
					{
						if (ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[0].GetLastKey().Time > MaxKeyTime)
							MaxKeyTime = ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[0].GetLastKey().Time;
					}
					if (ThisTrack.Value.CurveInfo.GKeyInfo.Num() > 0)
					{
						if (ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[1].GetLastKey().Time > MaxKeyTime)
							MaxKeyTime = ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[1].GetLastKey().Time;
					}
					if (ThisTrack.Value.CurveInfo.BKeyInfo.Num() > 0)
					{
						if (ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[2].GetLastKey().Time > MaxKeyTime)
							MaxKeyTime = ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[2].GetLastKey().Time;
					}
					if (ThisTrack.Value.CurveInfo.AKeyInfo.Num() > 0)
					{
						if (ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[3].GetLastKey().Time > MaxKeyTime)
							MaxKeyTime = ThisTrack.Value.CurveInfo.CurveObj->FloatCurves[3].GetLastKey().Time;
					}
				}
			}
		}
		#pragma endregion LinearColorTrack
	}
	else PrintError("GetMinKeyframeTime");
	return MaxKeyTime;
}

// Finished TODO:Wait Valid
float UAdvancedTimelineComponent::GetTimelineLength(const FString InTimelineGuName)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return 0.0f;
	}

	FTimelineInfo* ThisTimeline;
	if (QueryTimeline(InTimelineGuName, ThisTimeline))
	{
		FTimelineSetting* ThisSetting;
		if (QuerySetting(ThisTimeline->SettingGuName, ThisSetting)) {
			return ThisSetting->Length;
		}
		PrintError("GetTimelineLength");
		return 0.0f;
	}
	PrintError("GetTimelineLength");
	return 0.0f;
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::AddUpdateCallback(const FString InTimelineGuName, const FOnTimelineEvent InUpdateEvent)
{
	if (InTimelineGuName.IsEmpty() || !InUpdateEvent.IsBound())
	{
		PrintEmptyError();
		return;
	}

	FTimelineInfo* ThisTimeline;
	if (QueryTimeline(InTimelineGuName, ThisTimeline)) {
		FTimelineSetting* ThisSetting;
		if (QuerySetting(ThisTimeline->SettingGuName, ThisSetting)) {
			ThisSetting->EventFunc.OnUpdateEventFunc = InUpdateEvent;
		}
		PrintError("AddUpdateCallback");
		return;
	}
	PrintError("AddUpdateCallback");
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::AddFinishedCallback(const FString InTimelineGuName, const FOnTimelineEvent InFinishedEvent)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}

	FTimelineInfo* ThisTimeline;
	if (QueryTimeline(InTimelineGuName, ThisTimeline)) {
		FTimelineSetting* ThisSetting;
		if (QuerySetting(ThisTimeline->SettingGuName, ThisSetting)) {
			ThisSetting->EventFunc.OnFinishedEventFunc = InFinishedEvent;
		}
		PrintError("AddFinishedCallback");
		return;
	}
	PrintError("AddFinishedCallback");
}

// Finished
void UAdvancedTimelineComponent::AddTimelineInfo(const FTimelineInfo InTimeline,const FTimelineSetting InSetting)
{
	if (!ValidFTimelineInfo(&InTimeline))
	{
		PrintError(LOCTEXT("AddTimelineInfoError", 
					"There is an error in the TimelineInfo passed in, please check if you filled in the required fields!"));
		return;
	}
	Timelines.Add(InTimeline.GuName,InTimeline);
	SettingList.Add(InTimeline.SettingGuName, InSetting);
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::SaveSetting(const FTimelineSetting InTimelineSetting)
{
	if (ValidFTimelineSetting(&InTimelineSetting))
	{
		PrintError(LOCTEXT("SaveSettingError",
					"There is an error in the TimelineSetting passed in, please check if you filled in the required fields!"));
		return;
	}
	SettingList.Add(InTimelineSetting.GuName,InTimelineSetting);
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::AddFloatTrack(const FString InTimelineGuName, FFloatTrackInfo InFloatTrack)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}
	if (ValidFTrackInfo(&InFloatTrack))
	{
		PrintError(LOCTEXT("AddFloatTrackError",
			"There is an error in the FloatTrack passed in, please check if you filled in the required fields!"));
		return;
	}

	FTimelineInfo* ThisTimeline;
	
	if (QueryTimeline(InTimelineGuName, ThisTimeline)) {
		ThisTimeline->FloatTracks.Add(InFloatTrack.GuName, InFloatTrack);
	}
	PrintError("AddFloatTrack");
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::AddEventTrack(const FString InTimelineGuName, FEventTrackInfo InEventTrack)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}
	if (ValidFTrackInfo(&InEventTrack))
	{
		PrintError(LOCTEXT("AddEventTrackError",
			"There is an error in the EventTrack passed in, please check if you filled in the required fields!"));
		return;
	}

	FTimelineInfo* ThisTimeline;
	
	if (QueryTimeline(InTimelineGuName, ThisTimeline)) {
		ThisTimeline->EventTracks.Add(InEventTrack.GuName, InEventTrack);
	}
	PrintError("AddEventTrack");
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::AddVectorTrack(const FString InTimelineGuName, FVectorTrackInfo InVectorTrack)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}
	if (ValidFTrackInfo(&InVectorTrack))
	{
		PrintError(LOCTEXT("AddVectorTrackError",
			"There is an error in the VectorTrack passed in, please check if you filled in the required fields!"));
		return;
	}

	FTimelineInfo* ThisTimeline;
	
	if (QueryTimeline(InTimelineGuName, ThisTimeline)) {
		ThisTimeline->VectorTracks.Add(InVectorTrack.GuName, InVectorTrack);
	}
	PrintError("AddVectorTrack");
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::AddLinearColorTrack(const FString InTimelineGuName, FLinearColorTrackInfo InLinearColorTrack)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}
	if (ValidFTrackInfo(&InLinearColorTrack))
	{
		PrintError(LOCTEXT("AddLinearColorTrackError",
			"There is an error in the LinearColorTrack passed in, please check if you filled in the required fields!"));
		return;
	}

	FTimelineInfo* ThisTimeline;
	
	if (QueryTimeline(InTimelineGuName, ThisTimeline)) {
		ThisTimeline->LinearColorTracks.Add(InLinearColorTrack.GuName, InLinearColorTrack);
	}
	PrintError("AddLinearColorTrack");
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::GenericAddTrack(
	const FString InTimelineGuName,
	const EAdvTimelineType InTrackType,
	const UScriptStruct* StructType, 
	void* InTrack)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}

	FTimelineInfo* ThisTimeline;

	if (QueryTimeline(InTimelineGuName, ThisTimeline))
	{
		#pragma region
		if (Cast<UStruct>(StructType) == FFloatTrackInfo::StaticStruct())
		{
			FFloatTrackInfo* Float = (FFloatTrackInfo*)(InTrack);
			if (ValidFTrackInfo(Float))
			{
				PrintError(LOCTEXT("GenericAddTrackError1",
					"There is an error in the incoming FloatTrack, \
								please make sure that the track type is correct and that the required parameters are indeed filled in!"));
				return;
			}
			AddFloatTrack(InTimelineGuName, *Float);
		}
		#pragma endregion FloatTrack

		#pragma region
		if (Cast<UStruct>(StructType) == FEventTrackInfo::StaticStruct())
		{
			FEventTrackInfo* Event = (FEventTrackInfo*)(InTrack);
			if (ValidFTrackInfo(Event))
			{
				PrintError(LOCTEXT("GenericAddTrackError1",
					"There is an error in the incoming FloatTrack, \
								please make sure that the track type is correct and that the required parameters are indeed filled in!"));
				return;
			}
			AddEventTrack(InTimelineGuName, *Event);
		}
		#pragma endregion EventTrack

		#pragma region
		if (Cast<UStruct>(StructType) == FVectorTrackInfo::StaticStruct())
		{
			FVectorTrackInfo* Vector = (FVectorTrackInfo*)(InTrack);
			if (ValidFTrackInfo(Vector))
			{
				PrintError(LOCTEXT("GenericAddTrackError1",
					"There is an error in the incoming FloatTrack, \
								please make sure that the track type is correct and that the required parameters are indeed filled in!"));
				return;
			}
			AddVectorTrack(InTimelineGuName, *Vector);
		}
		#pragma endregion VectorTrack

		#pragma region
		if (Cast<UStruct>(StructType) == FLinearColorTrackInfo::StaticStruct())
		{
			FLinearColorTrackInfo* LinearColor = (FLinearColorTrackInfo*)(InTrack);
			if (ValidFTrackInfo(LinearColor))
			{
				PrintError(LOCTEXT("GenericAddTrackError1",
					"There is an error in the incoming FloatTrack, \
								please make sure that the track type is correct and that the required parameters are indeed filled in!"));
				return;
			}
			AddLinearColorTrack(InTimelineGuName, *LinearColor);
		}
		#pragma endregion LinearColorTrack
	}
	PrintError("GenericAddTrack");
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::SetLength(const FString InTimelineGuName, const float InNewTime)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}

	FTimelineInfo* ThisTimeline;
	if (QueryTimeline(InTimelineGuName, ThisTimeline)) {
		FTimelineSetting* ThisSetting;
		if (QuerySetting(ThisTimeline->SettingGuName, ThisSetting)) {
			ThisSetting->Length = InNewTime;
		}
		PrintError("SetLength");
		return;
	}
	PrintError("SetLength");
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::SetIgnoreTimeDilation(const FString InTimelineGuName, const bool IsIgnoreTimeDilation)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}

	FTimelineInfo* ThisTimeline;
	if (QueryTimeline(InTimelineGuName, ThisTimeline)) {
		FTimelineSetting* ThisSetting;
		if (QuerySetting(ThisTimeline->SettingGuName, ThisSetting)) {
			ThisSetting->bIgnoreTimeDilation = IsIgnoreTimeDilation;
		}
		PrintError("SetIgnoreTimeDilation");
		return;
	}
	PrintError("SetIgnoreTimeDilation");
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::SetPlaybackPosition(
	const FString InTimelineGuName,
	const float InNewTime,
	const bool bIsFireEvent, 
	const bool bIsFireUpdate, 
	const bool bIsFirePlay)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}

	FTimelineInfo* ThisTimeline;
	QueryTimeline(InTimelineGuName, ThisTimeline);

	if (ValidFTimelineInfo(ThisTimeline))
	{
		const float OldPosition = ThisTimeline->Position;
		ThisTimeline->Position = InNewTime;

		// Iterate over each vector interpolation
		for (const TPair<FString,FVectorTrackInfo> CurrentEntry : ThisTimeline->VectorTracks)
		{
			if (CurrentEntry.Value.CurveInfo.CurveObj && CurrentEntry.Value.OnEventFunc.IsBound())
			{
				// Get vector from curve
				FVector const Vec = CurrentEntry.Value.CurveInfo.CurveObj->GetVectorValue(GetPlaybackPosition(ThisTimeline->GuName));

				// Pass vec to specified function
				CurrentEntry.Value.OnEventFunc.ExecuteIfBound(Vec);
			}
		}

		// Iterate over each float interpolation
		for (const TPair < FString, FFloatTrackInfo> CurrentEntry : ThisTimeline->FloatTracks)
		{
			if (CurrentEntry.Value.CurveInfo.CurveObj && CurrentEntry.Value.OnEventFunc.IsBound())
			{
				// Get float from func
				const float Val = CurrentEntry.Value.CurveInfo.CurveObj->GetFloatValue(GetPlaybackPosition(ThisTimeline->GuName));

				// Pass float to specified function
				CurrentEntry.Value.OnEventFunc.ExecuteIfBound(Val);
			}
		}

		// Iterate over each color interpolation
		for (const TPair < FString, FLinearColorTrackInfo> CurrentEntry : ThisTimeline->LinearColorTracks)
		{
			if (CurrentEntry.Value.CurveInfo.CurveObj && CurrentEntry.Value.OnEventFunc.IsBound())
			{
				// Get vector from curve
				const FLinearColor Color = CurrentEntry.Value.CurveInfo.CurveObj->GetLinearColorValue(GetPlaybackPosition(ThisTimeline->GuName));

				// Pass vec to specified function
				CurrentEntry.Value.OnEventFunc.ExecuteIfBound(Color);
			}
		}


		// If we should be firing events for this track...
		if (bIsFireEvent)
		{
			// If playing sequence forwards.
			float MinTime, MaxTime;
			if (!ThisTimeline->bReversePlayback)
			{
				MinTime = OldPosition;
				MaxTime = ThisTimeline->Position;

				// Slight hack here.. if playing forwards and reaching the end of the sequence, force it over a little to ensure we fire events actually on the end of the sequence.
				if (MaxTime == GetTimelineLength(ThisTimeline->GuName))
				{
					MaxTime += static_cast<float>(KINDA_SMALL_NUMBER);
				}
			}
			// If playing sequence backwards.
			else
			{
				MinTime = ThisTimeline->Position;
				MaxTime = OldPosition;

				// Same small hack as above for backwards case.
				if (MinTime == 0.f)
				{
					MinTime -= static_cast<float>(KINDA_SMALL_NUMBER);
				}
			}

			// See which events fall into traversed region.
			for (const TPair < FString, FEventTrackInfo> ThisEventTrack : ThisTimeline->EventTracks)
			{
				for (auto It(ThisEventTrack.Value.CurveInfo.CurveObj->FloatCurve.GetKeyIterator()); It; ++It)
				{
					float EventTime = It->Time;

					// Need to be slightly careful here and make behavior for firing events symmetric when playing forwards of backwards.
					bool bFireThisEvent = false;
					if (!ThisTimeline->bReversePlayback)
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
						ThisEventTrack.Value.OnEventFunc.ExecuteIfBound();
					}
				}
			}
		}
		if (bIsFirePlay)
		{
			Play(ThisTimeline->GuName,EPlayMethod::Play);
		}
		if (bIsFireUpdate)
		{
			FTimelineSetting* ThisSetting;
			
			if (QuerySetting(ThisTimeline->SettingGuName, ThisSetting)) {
				ThisSetting->EventFunc.OnUpdateEventFunc.ExecuteIfBound();
			}
			// 因为后面没有其他逻辑了，所以直接Return就行，不然会输出两遍Error
			PrintError("SetPlaybackPosition");
			return;
		}
	}
	PrintError("SetPlaybackPosition");
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::SetLengthMode(const FString InTimelineGuName, const ELengthMode InLengthMode)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}

	FTimelineInfo* ThisTimeline;
	if (QueryTimeline(InTimelineGuName, ThisTimeline)) {
		FTimelineSetting* ThisSetting;
		if (QuerySetting(ThisTimeline->SettingGuName, ThisSetting)) {
			ThisSetting->LengthMode = InLengthMode;
		}
		PrintError("SetLengthMode");
		return;
	}
	PrintError("SetLengthMode");
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::SetLoop(const FString InTimelineGuName, const bool bIsLoop)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}

	FTimelineInfo* ThisTimeline;	
	if (QueryTimeline(InTimelineGuName, ThisTimeline)) {
		FTimelineSetting* ThisSetting;
		if (QuerySetting(ThisTimeline->SettingGuName, ThisSetting)) {
			ThisSetting->bIsLoop = bIsLoop;
		}
		PrintError("SetLoop");
		return;
	}
	PrintError("SetLoop");
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::SetPlayRate(const FString InTimelineGuName, const float InNewPlayRate)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}

	FTimelineInfo* ThisTimeline;
	if (QueryTimeline(InTimelineGuName, ThisTimeline)) {
		FTimelineSetting* ThisSetting;
		if (QuerySetting(ThisTimeline->SettingGuName, ThisSetting)) {
			ThisSetting->PlayRate = InNewPlayRate;
		}
		PrintError("SetPlayRate");
		return;
	}
	PrintError("SetPlayRate");
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::ChangeTrackCurve(const FString InTrackGuName, UCurveBase* InCurve)
{
	if (InTrackGuName.IsEmpty() || !InCurve)
	{
		PrintEmptyError();
		return;
	}

	FTimelineInfo* TimelineInfo;
	EAdvTimelineType TrackType;
	FTrackInfoBase* BaseTrack;
	if (QueryTrackByGuid(InTrackGuName, TrackType, BaseTrack, TimelineInfo))
	{
		switch (TrackType)
		{
				#pragma region
			case EAdvTimelineType::Float:
				if (FFloatTrackInfo* FloatTrack = (FFloatTrackInfo*)(BaseTrack))
				{
					if (ValidFTrackInfo(FloatTrack))
					{
						FloatTrack->CurveInfo.CurveObj = Cast<UCurveFloat>(InCurve);
						FloatTrack->CurveInfo.CurveType = EAdvTimelineType::Float;
						if (FloatTrack->CurveInfo.CurveObj->FloatCurve.GetNumKeys() > 0)
						{
							for (auto It(FloatTrack->CurveInfo.CurveObj->FloatCurve.GetKeyIterator()); It; ++It)
							{
								FKeyInfo ThisKey;
								ThisKey.Guid = UKismetGuidLibrary::NewGuid().ToString();
								EInterpCurveMode InterpMode = CIM_Unknown;
								FKeyInfo::ConversionKeyModeToInterpMode(*It, InterpMode);
								ThisKey.InterpMode = InterpMode;
								ThisKey.BaseKey = *It;
								FloatTrack->CurveInfo.CurveKeyInfo.Add(ThisKey);
							}
						}
					}
				}
				break;
				#pragma endregion Float

				#pragma region
			case EAdvTimelineType::Event:
				if (FEventTrackInfo* EventTrack = (FEventTrackInfo*)(BaseTrack))
				{
					if (ValidFTrackInfo(EventTrack))
					{
						EventTrack->CurveInfo.CurveObj = Cast<UCurveFloat>(InCurve);
						EventTrack->CurveInfo.CurveType = EAdvTimelineType::Event;
						if (EventTrack->CurveInfo.CurveObj->FloatCurve.GetNumKeys() > 0)
						{
							for (auto It(EventTrack->CurveInfo.CurveObj->FloatCurve.GetKeyIterator()); It; ++It)
							{
								FEventKey ThisKey;
								ThisKey.Guid = UKismetGuidLibrary::NewGuid().ToString();
								ThisKey.BaseKey = *It;
								ThisKey.Time = It->Time;
								EventTrack->CurveInfo.EventKey.Add(ThisKey);
							}
						}
					}
				}
				break;
				#pragma endregion Event

				#pragma region
			case EAdvTimelineType::Vector:
				if (FVectorTrackInfo* VectorTrack = (FVectorTrackInfo*)(BaseTrack))
				{
					if (ValidFTrackInfo(VectorTrack))
					{
						VectorTrack->CurveInfo.CurveObj = Cast < UCurveVector>(InCurve);
						VectorTrack->CurveInfo.CurveType = EAdvTimelineType::Vector;
						if (VectorTrack->CurveInfo.CurveObj->FloatCurves[0].GetNumKeys() > 0)
						{
							for (auto It(VectorTrack->CurveInfo.CurveObj->FloatCurves[0].GetKeyIterator()); It; ++It)
							{
								FKeyInfo ThisKey;
								ThisKey.Guid = UKismetGuidLibrary::NewGuid().ToString();
								EInterpCurveMode InterpMode = CIM_Unknown;
								FKeyInfo::ConversionKeyModeToInterpMode(*It, InterpMode);
								ThisKey.InterpMode = InterpMode;
								ThisKey.BaseKey = *It;
								VectorTrack->CurveInfo.XKeyInfo.Add(ThisKey);
							}
							for (auto It(VectorTrack->CurveInfo.CurveObj->FloatCurves[1].GetKeyIterator()); It; ++It)
							{
								FKeyInfo ThisKey;
								ThisKey.Guid = UKismetGuidLibrary::NewGuid().ToString();
								EInterpCurveMode InterpMode = CIM_Unknown;
								FKeyInfo::ConversionKeyModeToInterpMode(*It, InterpMode);
								ThisKey.InterpMode = InterpMode;
								ThisKey.BaseKey = *It;
								VectorTrack->CurveInfo.YKeyInfo.Add(ThisKey);
							}
							for (auto It(VectorTrack->CurveInfo.CurveObj->FloatCurves[2].GetKeyIterator()); It; ++It)
							{
								FKeyInfo ThisKey;
								ThisKey.Guid = UKismetGuidLibrary::NewGuid().ToString();
								EInterpCurveMode InterpMode = CIM_Unknown;
								FKeyInfo::ConversionKeyModeToInterpMode(*It, InterpMode);
								ThisKey.InterpMode = InterpMode;
								ThisKey.BaseKey = *It;
								VectorTrack->CurveInfo.ZKeyInfo.Add(ThisKey);
							}
						}
					}
				}
				break;
				#pragma endregion Vector

				#pragma region
			case EAdvTimelineType::LinearColor:
				if (FLinearColorTrackInfo* LinearColorTrack = (FLinearColorTrackInfo*)(BaseTrack))
				{
					if (ValidFTrackInfo(LinearColorTrack))
					{
						LinearColorTrack->CurveInfo.CurveObj = Cast<UCurveLinearColor>(InCurve);
						LinearColorTrack->CurveInfo.CurveType = EAdvTimelineType::Float;
						if (LinearColorTrack->CurveInfo.CurveObj->FloatCurves[0].GetNumKeys() > 0)
						{
							for (auto It(LinearColorTrack->CurveInfo.CurveObj->FloatCurves[0].GetKeyIterator()); It; ++It)
							{
								FKeyInfo ThisKey;
								ThisKey.Guid = UKismetGuidLibrary::NewGuid().ToString();
								EInterpCurveMode InterpMode = CIM_Unknown;
								FKeyInfo::ConversionKeyModeToInterpMode(*It, InterpMode);
								ThisKey.InterpMode = InterpMode;
								ThisKey.BaseKey = *It;
								LinearColorTrack->CurveInfo.RKeyInfo.Add(ThisKey);
							}
							for (auto It(LinearColorTrack->CurveInfo.CurveObj->FloatCurves[1].GetKeyIterator()); It; ++It)
							{
								FKeyInfo ThisKey;
								ThisKey.Guid = UKismetGuidLibrary::NewGuid().ToString();
								EInterpCurveMode InterpMode = CIM_Unknown;
								FKeyInfo::ConversionKeyModeToInterpMode(*It, InterpMode);
								ThisKey.InterpMode = InterpMode;
								ThisKey.BaseKey = *It;
								LinearColorTrack->CurveInfo.GKeyInfo.Add(ThisKey);
							}
							for (auto It(LinearColorTrack->CurveInfo.CurveObj->FloatCurves[2].GetKeyIterator()); It; ++It)
							{
								FKeyInfo ThisKey;
								ThisKey.Guid = UKismetGuidLibrary::NewGuid().ToString();
								EInterpCurveMode InterpMode = CIM_Unknown;
								FKeyInfo::ConversionKeyModeToInterpMode(*It, InterpMode);
								ThisKey.InterpMode = InterpMode;
								ThisKey.BaseKey = *It;
								LinearColorTrack->CurveInfo.BKeyInfo.Add(ThisKey);
							}
							for (auto It(LinearColorTrack->CurveInfo.CurveObj->FloatCurves[3].GetKeyIterator()); It; ++It)
							{
								FKeyInfo ThisKey;
								ThisKey.Guid = UKismetGuidLibrary::NewGuid().ToString();
								EInterpCurveMode InterpMode = CIM_Unknown;
								FKeyInfo::ConversionKeyModeToInterpMode(*It, InterpMode);
								ThisKey.InterpMode = InterpMode;
								ThisKey.BaseKey = *It;
								LinearColorTrack->CurveInfo.AKeyInfo.Add(ThisKey);
							}
						}
					}
				}
				break;
				#pragma endregion LinearColor
		}
	}
	PrintError("ChangeTrackCurve");
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::ResetTimeline(const FString InTimelineGuName)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}

	FTimelineInfo* ThisTimeline;
	if (QueryTimeline(InTimelineGuName, ThisTimeline)) {
		ThisTimeline->EventTracks.Empty();
		ThisTimeline->VectorTracks.Empty();
		ThisTimeline->FloatTracks.Empty();
		ThisTimeline->LinearColorTracks.Empty();
		ThisTimeline->IsPlaying = false;
		ThisTimeline->IsStop = true;
		ThisTimeline->PlayStatus = EPlayStatus::Stop;
		ThisTimeline->Position = 0.0f;

		Timelines.Add(ThisTimeline->GuName, *ThisTimeline);
	}
	PrintError("ResetTimeline");
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::ClearTimelineTracks(const FString InTimelineGuName)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}

	FTimelineInfo* ThisTimeline;	
	if (QueryTimeline(InTimelineGuName, ThisTimeline)) {
		ThisTimeline->EventTracks.Empty();
		ThisTimeline->FloatTracks.Empty();
		ThisTimeline->VectorTracks.Empty();
		ThisTimeline->LinearColorTracks.Empty();
	}
	PrintError("ClearTimelineTracks");
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::ResetTrack(const FString InTrackGuName)
{
	if (InTrackGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}

	FTimelineInfo* TimelineInfo;
	EAdvTimelineType TrackType;
	FTrackInfoBase* BaseTrack = nullptr;
	if (QueryTrackByGuid(InTrackGuName, TrackType, BaseTrack, TimelineInfo)) {
		switch (TrackType)
		{
				#pragma region
			case EAdvTimelineType::Float:
				if (FFloatTrackInfo* FloatTrack = (FFloatTrackInfo*)BaseTrack)
				{
					FloatTrack->CurveInfo.CurveKeyInfo.Empty();
					FloatTrack->CurveInfo.CurveObj = nullptr;
					FloatTrack->OnEventFunc.Clear();
				}
				break;
				#pragma endregion Float

				#pragma region
			case EAdvTimelineType::Event:
				if (FEventTrackInfo* EventTrack = (FEventTrackInfo*)BaseTrack)
				{
					EventTrack->CurveInfo.EventKey.Empty();
					EventTrack->CurveInfo.CurveObj = nullptr;
					EventTrack->OnEventFunc.Clear();
				}
				break;
				#pragma endregion Event

				#pragma region
			case EAdvTimelineType::Vector:
				if (FVectorTrackInfo* VectorTrack = (FVectorTrackInfo*)BaseTrack)
				{
					VectorTrack->CurveInfo.XKeyInfo.Empty();
					VectorTrack->CurveInfo.YKeyInfo.Empty();
					VectorTrack->CurveInfo.ZKeyInfo.Empty();
					VectorTrack->CurveInfo.CurveObj = nullptr;
					VectorTrack->OnEventFunc.Clear();
				}
				break;
				#pragma endregion Vector

				#pragma region
			case EAdvTimelineType::LinearColor:
				if (FLinearColorTrackInfo* LinearColorTrack = (FLinearColorTrackInfo*)BaseTrack)
				{
					LinearColorTrack->CurveInfo.RKeyInfo.Empty();
					LinearColorTrack->CurveInfo.GKeyInfo.Empty();
					LinearColorTrack->CurveInfo.BKeyInfo.Empty();
					LinearColorTrack->CurveInfo.AKeyInfo.Empty();
					LinearColorTrack->CurveInfo.CurveObj = nullptr;
					LinearColorTrack->OnEventFunc.Clear();
				}
				break;
				#pragma endregion LinearColor
		}
	}
	PrintError("ResetTrack");
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::DelTrack(const FString InTrackGuName)
{
	if (InTrackGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}

	FTimelineInfo* TimelineInfo;
	EAdvTimelineType TrackType;
	FTrackInfoBase* BaseTrack = nullptr;
	if (QueryTrackByGuid(InTrackGuName, TrackType, BaseTrack, TimelineInfo)) {
		switch (TrackType)
		{
			case EAdvTimelineType::Float:
				if (const FFloatTrackInfo* FloatTrack = (FFloatTrackInfo*)(BaseTrack))
				{
					TimelineInfo->FloatTracks.Remove(FloatTrack->GuName);
				}
				break;
			case EAdvTimelineType::Event:
				if (const FEventTrackInfo* EventTrack = (FEventTrackInfo*)(BaseTrack))
				{
					TimelineInfo->EventTracks.Remove(EventTrack->GuName);
				}
				break;
			case EAdvTimelineType::Vector:
				if (const FVectorTrackInfo* VectorTrack = (FVectorTrackInfo*)(BaseTrack))
				{
					TimelineInfo->VectorTracks.Remove(VectorTrack->GuName);
				}
				break;
			case EAdvTimelineType::LinearColor:
				if (const FLinearColorTrackInfo* LinearColorTrack = (FLinearColorTrackInfo*)(BaseTrack))
				{
					TimelineInfo->LinearColorTracks.Remove(LinearColorTrack->GuName);
				}
				break;
		}
	}
	PrintError("DelTrack");
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::ApplySettingToTimeline(
	const FString InSettingsName,
	const FString InTimelineGuName,
	FTimelineSetting& OutOriginSettings)
{
	if (InSettingsName.IsEmpty() || InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}

	FTimelineInfo* ThisTimeline;
	if (QueryTimeline(InTimelineGuName, ThisTimeline)) {
		FTimelineSetting* OriginSetting;
		if (QuerySetting(ThisTimeline->SettingGuName, OriginSetting)) {
			FTimelineSetting* ThisSetting;
			OutOriginSettings = *OriginSetting;
			if (QuerySetting(InSettingsName, ThisSetting)) {
				ThisTimeline->SettingGuName = ThisSetting->GuName;
				Timelines.Add(ThisTimeline->GuName, *ThisTimeline);
				return;
			}
			PrintError("ApplySettingToTimeline");
			return;
		}
	}
	PrintError("ApplySettingToTimeline");
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::DelTimeline(const FString InTimelineGuName)
{
	if (!InTimelineGuName.IsEmpty() && Timelines.Num() > 1)
	{
		FTimelineInfo* ThisTimeline;
		if (QueryTimeline(InTimelineGuName, ThisTimeline)) {
			Timelines.Remove(ThisTimeline->GuName);
			SettingList.Remove(ThisTimeline->SettingGuName);
			return;
		}
		PrintError("DelTimeline");
		return;
	}
	PrintError(LOCTEXT("DelTimelineError","There is only one timeline left! Or the input parameter is empty!"));
}

// Finished
void UAdvancedTimelineComponent::ClearTimelines()
{
	Timelines.Empty();
	FTimelineInfo ThisTimeline;
	FTimelineSetting ThisSetting;
	GenerateDefaultTimeline(ThisTimeline, ThisSetting);
	AddTimelineInfo(ThisTimeline, ThisSetting);
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::ClearSettingList()
{
	SettingList.Empty();
	FTimelineInfo ThisTimeline;
	FTimelineSetting ThisSetting;
	GenerateDefaultTimeline(ThisTimeline, ThisSetting);
	SaveSetting(ThisSetting);

}

// Finished TODO:Wait Valid
bool UAdvancedTimelineComponent::GetTimeline(const FString InTimelineGuName, FTimelineInfo& OutTimelineInfo)
{
	if (!InTimelineGuName.IsEmpty() && (Timelines.Num() > 0))
	{
		if (const FTimelineInfo* ThisTimeline = Timelines.Find(InTimelineGuName)) {
			if (ThisTimeline->GuName == InTimelineGuName)
			{
				OutTimelineInfo = *ThisTimeline;
				return true;
			}
		}
	}
	PrintError(LOCTEXT("GetTimelineError", 
		"Please confirm the number of Timeline and that there must be one and only one Guid and Name."));
	return false;
}

// Finished TODO:Wait Valid
bool UAdvancedTimelineComponent::GetSetting(const FString InSettingsGuName, FTimelineSetting& OutSetting)
{
	if (!InSettingsGuName.IsEmpty() && (SettingList.Num() > 0))
	{
		if (const FTimelineSetting* ThisSetting = SettingList.Find(InSettingsGuName)) {
			if (ThisSetting->GuName == InSettingsGuName)
			{
				OutSetting = *ThisSetting;
				return true;
			}
		}
	}
	PrintError(LOCTEXT("GetSettingError", 
		"Please confirm the number of SettingList and that there must be one and only one Guid and Name."));
	return false;
}

// Finished 
bool UAdvancedTimelineComponent::QueryTimeline(const FString InTimelineGuName, FTimelineInfo*& OutTimelineInfo)
{
	if (!InTimelineGuName.IsEmpty() && (Timelines.Num() > 0))
	{
		if (FTimelineInfo* ThisTimeline = Timelines.Find(InTimelineGuName))
		{
			OutTimelineInfo = ThisTimeline;
			return true;
		}
	}
	PrintError(LOCTEXT("GetTimelineError",
		"Please confirm the number of Timeline and that there must be one and only one Guid and Name."));
	return false;
}

// Finished 
bool UAdvancedTimelineComponent::QuerySetting(const FString InSettingsGuName, FTimelineSetting* & OutSetting)
{
	if (!InSettingsGuName.IsEmpty() && (SettingList.Num() > 0))
	{
		if (FTimelineSetting* ThisSetting = SettingList.Find(InSettingsGuName))
		{
			OutSetting = ThisSetting;
			return true;
		}
	}
	PrintError(LOCTEXT("GetSettingError",
		"Please confirm the number of SettingList and that there must be one and only one Guid and Name."));
	return false;
}

// Finished TODO:Wait Valid
bool UAdvancedTimelineComponent::QueryTrackByGuid(const FString InTrackGuName, EAdvTimelineType& OutTrackType,
	FTrackInfoBase*& OutTrack, FTimelineInfo*& OutTimeline)
{
	if (!InTrackGuName.IsEmpty() && (Timelines.Num() > 0))
	{
		for (TPair<FString, FTimelineInfo>& ThisTimeline : Timelines)
		{
			if (GetTrackCount(ThisTimeline.Value.GuName) > 0) {

				#pragma region
				if (ThisTimeline.Value.FloatTracks.Num() > 0)
				{
					if (FFloatTrackInfo* ThisFloatTrack = ThisTimeline.Value.FloatTracks.Find(InTrackGuName)) {
						OutTrackType = EAdvTimelineType::Float;
						OutTrack = static_cast<FTrackInfoBase*>(ThisFloatTrack);
						OutTimeline = &ThisTimeline.Value;
						return true;
					}
				}
				#pragma endregion FloatTracks

				#pragma region
				if (ThisTimeline.Value.EventTracks.Num() > 0)
				{
					if (FEventTrackInfo* ThisEventTrack = ThisTimeline.Value.EventTracks.Find(InTrackGuName)) {
						OutTrackType = EAdvTimelineType::Event;
						OutTrack = static_cast<FTrackInfoBase*>(ThisEventTrack);
						OutTimeline = &ThisTimeline.Value;
						return true;
					}
				}
				#pragma endregion EventTracks

				#pragma region
				if (ThisTimeline.Value.VectorTracks.Num() > 0)
				{
					if (FVectorTrackInfo* ThisVectorTrack = ThisTimeline.Value.VectorTracks.Find(InTrackGuName)) {
						OutTrackType = EAdvTimelineType::Vector;
						OutTrack = static_cast<FTrackInfoBase*>(ThisVectorTrack);
						OutTimeline = &ThisTimeline.Value;
						return true;
					}
				}
				#pragma endregion VectorTracks

				#pragma region
				if (ThisTimeline.Value.LinearColorTracks.Num() > 0)
				{
					if (FLinearColorTrackInfo* ThisLinearColorTrack = ThisTimeline.Value.LinearColorTracks.Find(InTrackGuName)) {
						OutTrackType = EAdvTimelineType::LinearColor;
						OutTrack = static_cast<FTrackInfoBase*>(ThisLinearColorTrack);
						OutTimeline = &ThisTimeline.Value;
						return true;
					}
				}
				#pragma endregion LinearColorTracks
			}
		}
	}
	PrintError(LOCTEXT("TrackQueryError", 
		"No track found or the track to be found is empty or there is no timeline or the number of tracks is 0!"));
	return false;
}

// Finished TODO:Wait Valid
bool UAdvancedTimelineComponent::QueryCurveByGuid(const FString InCurveGuid, EAdvTimelineType& OutCurveType,
	FCurveInfoBase*& OutCurve, FTimelineInfo*& OutTimeline)
{
	if (!InCurveGuid.IsEmpty() && Timelines.Num() > 0)
	{
		for (TPair<FString, FTimelineInfo>& ThisTimeline : Timelines)
		{
			if (GetTrackCount(ThisTimeline.Value.GuName) > 0) {

				#pragma region
				if (ThisTimeline.Value.FloatTracks.Num() > 0)
				{
					for (TPair < FString, FFloatTrackInfo> ThisFloatTrack : ThisTimeline.Value.FloatTracks)
					{
						if (ThisFloatTrack.Value.CurveInfo.Guid == InCurveGuid)
						{
							OutCurveType = EAdvTimelineType::Float;
							OutCurve = &ThisFloatTrack.Value.CurveInfo;
							OutTimeline = &ThisTimeline.Value;
							return true;
						}
					}
				}
				#pragma endregion FloatTracks

				#pragma region
				if (ThisTimeline.Value.EventTracks.Num() > 0)
				{
					for (TPair < FString, FEventTrackInfo> ThisEventTrack : ThisTimeline.Value.EventTracks)
					{
						if (ThisEventTrack.Value.CurveInfo.Guid == InCurveGuid)
						{
							OutCurveType = EAdvTimelineType::Event;
							OutCurve = &ThisEventTrack.Value.CurveInfo;
							OutTimeline = &ThisTimeline.Value;
							return true;
						}
					}
				}
				#pragma endregion EventTracks

				#pragma region
				if (ThisTimeline.Value.VectorTracks.Num() > 0)
				{
					for (TPair < FString, FVectorTrackInfo> ThisVectorTrack : ThisTimeline.Value.VectorTracks)
					{
						if (ThisVectorTrack.Value.CurveInfo.Guid == InCurveGuid)
						{
							OutCurveType = EAdvTimelineType::Vector;
							OutCurve = &ThisVectorTrack.Value.CurveInfo;
							OutTimeline = &ThisTimeline.Value;
							return true;
						}
					}
				}
				#pragma endregion VectorTracks

				#pragma region
				if (ThisTimeline.Value.LinearColorTracks.Num() > 0)
				{
					for (TPair < FString, FLinearColorTrackInfo> ThisLinearColorTrack : ThisTimeline.Value.LinearColorTracks)
					{
						if (ThisLinearColorTrack.Value.CurveInfo.Guid == InCurveGuid)
						{
							OutCurveType = EAdvTimelineType::LinearColor;
							OutCurve = &ThisLinearColorTrack.Value.CurveInfo;
							OutTimeline = &ThisTimeline.Value;
							return true;
						}
					}
				}
				#pragma endregion LinearColorTracks
			}
		}
	}
	PrintError(LOCTEXT("CurveQueryError", 
		"No curve found or the curve to be found is empty or there is no timeline or the number of tracks is 0!"));
	return false;
}

// Finished TODO:Wait Valid
bool UAdvancedTimelineComponent::QueryKeyByGuid(const FString InKeyGuid, 
	EAdvTimelineType& OutKeyType, 
	FRichCurveKey*& OutKey,
	FTimelineInfo*& OutTimeline)
{
	if (!InKeyGuid.IsEmpty() && Timelines.Num() > 0)
	{
		for (TPair<FString, FTimelineInfo>& ThisTimeline : Timelines)
		{
			if (GetTrackCount(ThisTimeline.Value.GuName) > 0) {

				#pragma region
				if (ThisTimeline.Value.FloatTracks.Num() > 0)
				{
					for (TPair<FString,FFloatTrackInfo> ThisFloatTrack : ThisTimeline.Value.FloatTracks)
					{
						if (ThisFloatTrack.Value.CurveInfo.CurveKeyInfo.Num() > 0)
						{
							for (FKeyInfo ThisKey : ThisFloatTrack.Value.CurveInfo.CurveKeyInfo)
							{
								if (ThisKey.Guid == InKeyGuid)
								{
									OutKeyType = EAdvTimelineType::Float;
									OutKey = &ThisKey.BaseKey;
									OutTimeline = &ThisTimeline.Value;
									return true;
								}
							}
						}
					}
				}
				#pragma endregion FloatTracks

				#pragma region
				if (ThisTimeline.Value.EventTracks.Num() > 0)
				{
					for (TPair < FString, FEventTrackInfo> ThisEventTrack : ThisTimeline.Value.EventTracks)
					{
						if (ThisEventTrack.Value.CurveInfo.EventKey.Num() > 0)
						{
							for (FEventKey ThisKey : ThisEventTrack.Value.CurveInfo.EventKey)
							{
								if (ThisKey.Guid == InKeyGuid)
								{
									OutKeyType = EAdvTimelineType::Event;
									OutKey = &ThisKey.BaseKey;
									OutTimeline = &ThisTimeline.Value;
									return true;
								}
							}
						}
					}
				}
				#pragma endregion EventTracks

				#pragma region
				if (ThisTimeline.Value.VectorTracks.Num() > 0)
				{
					for (TPair < FString, FVectorTrackInfo> ThisVectorTrack : ThisTimeline.Value.VectorTracks)
					{
						if (ThisVectorTrack.Value.CurveInfo.XKeyInfo.Num() > 0)
						{
							for (FKeyInfo ThisKey : ThisVectorTrack.Value.CurveInfo.XKeyInfo)
							{
								if (ThisKey.Guid == InKeyGuid)
								{
									OutKeyType = EAdvTimelineType::Vector;
									OutKey = &ThisKey.BaseKey;
									OutTimeline = &ThisTimeline.Value;
									return true;
								}
							}
						}
						if (ThisVectorTrack.Value.CurveInfo.YKeyInfo.Num() > 0)
						{
							for (FKeyInfo ThisKey : ThisVectorTrack.Value.CurveInfo.YKeyInfo)
							{
								if (ThisKey.Guid == InKeyGuid)
								{
									OutKeyType = EAdvTimelineType::Vector;
									OutKey = &ThisKey.BaseKey;
									OutTimeline = &ThisTimeline.Value;
									return true;
								}
							}
						}
						if (ThisVectorTrack.Value.CurveInfo.ZKeyInfo.Num() > 0)
						{
							for (FKeyInfo ThisKey : ThisVectorTrack.Value.CurveInfo.ZKeyInfo)
							{
								if (ThisKey.Guid == InKeyGuid)
								{
									OutKeyType = EAdvTimelineType::Vector;
									OutKey = &ThisKey.BaseKey;
									OutTimeline = &ThisTimeline.Value;
									return true;
								}
							}
						}
					}
				}
				#pragma endregion VectorTracks

				#pragma region
				if (ThisTimeline.Value.LinearColorTracks.Num() > 0)
				{
					for (TPair < FString, FLinearColorTrackInfo> ThisLinearColorTrack : ThisTimeline.Value.LinearColorTracks)
					{
						if (ThisLinearColorTrack.Value.CurveInfo.RKeyInfo.Num() > 0)
						{
							for (FKeyInfo ThisKey : ThisLinearColorTrack.Value.CurveInfo.RKeyInfo)
							{
								if (ThisKey.Guid == InKeyGuid)
								{
									OutKeyType = EAdvTimelineType::LinearColor;
									OutKey = &ThisKey.BaseKey;
									OutTimeline = &ThisTimeline.Value;
									return true;
								}
							}
						}
						if (ThisLinearColorTrack.Value.CurveInfo.GKeyInfo.Num() > 0)
						{
							for (FKeyInfo ThisKey : ThisLinearColorTrack.Value.CurveInfo.GKeyInfo)
							{
								if (ThisKey.Guid == InKeyGuid)
								{
									OutKeyType = EAdvTimelineType::LinearColor;
									OutKey = &ThisKey.BaseKey;
									OutTimeline = &ThisTimeline.Value;
									return true;
								}
							}
						}
						if (ThisLinearColorTrack.Value.CurveInfo.BKeyInfo.Num() > 0)
						{
							for (FKeyInfo ThisKey : ThisLinearColorTrack.Value.CurveInfo.BKeyInfo)
							{
								if (ThisKey.Guid == InKeyGuid)
								{
									OutKeyType = EAdvTimelineType::LinearColor;
									OutKey = &ThisKey.BaseKey;
									OutTimeline = &ThisTimeline.Value;
									return true;
								}
							}
						}
						if (ThisLinearColorTrack.Value.CurveInfo.AKeyInfo.Num() > 0)
						{
							for (FKeyInfo ThisKey : ThisLinearColorTrack.Value.CurveInfo.AKeyInfo)
							{
								if (ThisKey.Guid == InKeyGuid)
								{
									OutKeyType = EAdvTimelineType::LinearColor;
									OutKey = &ThisKey.BaseKey;
									OutTimeline = &ThisTimeline.Value;
									return true;
								}
							}
						}
					}
				}
				#pragma endregion LinearColorTracks
			}
		}
	}
	PrintError(LOCTEXT("KeyQueryError",
		"No keyframe found or the keyframe to be found is empty or there is no timeline or the number of tracks is 0!"));
	return false;
}

// Finished TODO:Wait Valid
bool UAdvancedTimelineComponent::ValidFTimelineInfo(const FTimelineInfo* InTimelineInfo)
{
	if (InTimelineInfo &&
		!InTimelineInfo->GuName.IsEmpty() &&
		!InTimelineInfo->SettingGuName.IsEmpty())
	{
		#pragma region
		if (InTimelineInfo->EventTracks.Num() > 0)
		{
			for (TPair<FString,FEventTrackInfo> ThisTracks : InTimelineInfo->EventTracks)
			{
				if (!ValidFTrackInfo(&ThisTracks.Value)) return false;
			}
		}
		#pragma endregion EventTracks

		#pragma region
		if (InTimelineInfo->FloatTracks.Num() > 0)
		{
			for (TPair < FString, FFloatTrackInfo> ThisTracks : InTimelineInfo->FloatTracks)
			{
				if (!ValidFTrackInfo(&ThisTracks.Value)) return false;
			}
		}
		#pragma endregion FloatTracks

		#pragma region
		if (InTimelineInfo->VectorTracks.Num() > 0)
		{
			for (TPair < FString, FVectorTrackInfo> ThisTracks : InTimelineInfo->VectorTracks)
			{
				if (!ValidFTrackInfo(&ThisTracks.Value)) return false;
			}
		}
		#pragma endregion VectorTracks

		#pragma region
		if (InTimelineInfo->LinearColorTracks.Num() > 0)
		{
			for (TPair < FString, FLinearColorTrackInfo> ThisTracks : InTimelineInfo->LinearColorTracks)
			{
				if (!ValidFTrackInfo(&ThisTracks.Value)) return false;
			}
		}
		#pragma endregion LinearColorTracks

		return true;
	}

	PrintError(LOCTEXT("TimelineValidError", 
		"Please check that TimelineInfo is passed in and that it has an empty Guid and Name."));
	return false;
}

// Finished TODO:Wait Valid
bool UAdvancedTimelineComponent::ValidFTimelineSetting(const FTimelineSetting* InSettingsInfo)
{
	if (InSettingsInfo && !InSettingsInfo->GuName.IsEmpty())
	{
		return true;
	}

	PrintError(LOCTEXT("TimelineSettingValidError",
		"Please check that TimelineSetting is passed in and that its Guid and Name are not empty.\
					 Please note that if the Update and Finished functions are not filled in, \
					then using this setting Timeline will only use valid events on the track."));
	return false;
}

// Finished TODO:Wait Valid
bool UAdvancedTimelineComponent::ValidFTrackInfo(FTrackInfoBase* InTrack)
{
	if (!InTrack)
	{
		#pragma region
		if (const FFloatTrackInfo* FloatTrack = (FFloatTrackInfo*)(InTrack))
		{
			if (!FloatTrack->GuName.IsEmpty())
			{
				return true;
			}
		}
		#pragma endregion FloatTrack

		#pragma region
		if (const FEventTrackInfo* EventTrack = (FEventTrackInfo*)(InTrack))
		{
			if (!EventTrack->GuName.IsEmpty())
			{
				return true;
			}
		}
		#pragma endregion EventTrack

		#pragma region
		if (const FVectorTrackInfo* VectorTrack = (FVectorTrackInfo*)(InTrack))
		{
			if (!VectorTrack->GuName.IsEmpty())
			{
				return true;
			}
		}
		#pragma endregion VectorTrack

		#pragma region
		if (const FLinearColorTrackInfo* LinearColorTrack = (FLinearColorTrackInfo*)(InTrack))
		{
			if (!LinearColorTrack->GuName.IsEmpty())
			{
				return true;
			}
		}
		#pragma endregion LinearColorTrack
	}

	PrintError(LOCTEXT("TrackValidError", 
		"Please check that the track is passed in and that the track has the correct GUID, track name. \
					If you don't add curves or execute functions, then this track will only execute Update and Finished functions."));
	return false;
}

// Finished TODO:Wait Valid
bool UAdvancedTimelineComponent::ValidFCurveInfo(FCurveInfoBase* InCurve)
{
	if (!InCurve) {
		#pragma region
		if (const FFloatCurveInfo* FloatCurve = (FFloatCurveInfo*)(InCurve))
		{
			if (!FloatCurve->Guid.IsEmpty() && IsValid(FloatCurve->CurveObj))
			{
				if (FloatCurve->CurveKeyInfo.Num() > 0)
				{
					for (FKeyInfo ThisKey : FloatCurve->CurveKeyInfo)
					{
						if (!ValidFKeyInfo(&ThisKey)) return false;
					}
				}
				return true;
			}
		}
		#pragma endregion FloatCurve

		#pragma region
		if (const FEventCurveInfo* EventCurve = (FEventCurveInfo*)(InCurve))
		{
			if (!EventCurve->Guid.IsEmpty() && IsValid(EventCurve->CurveObj))
			{
				if (EventCurve->EventKey.Num() > 0)
				{
					for (FEventKey ThisKey : EventCurve->EventKey)
					{
						if (ThisKey.Guid.IsEmpty()) return false;
					}
				}
				return true;
			}
		}
		#pragma endregion EventCurve

		#pragma region
		if (const FVectorCurveInfo* VectorCurve = (FVectorCurveInfo*)(InCurve))
		{
			if (!VectorCurve->Guid.IsEmpty() && IsValid(VectorCurve->CurveObj))
			{
				if (VectorCurve->XKeyInfo.Num() > 0)
				{
					for (FKeyInfo ThisKey : VectorCurve->XKeyInfo)
					{
						if (!ValidFKeyInfo(&ThisKey)) return false;
					}
				}
				if (VectorCurve->YKeyInfo.Num() > 0)
				{
					for (FKeyInfo ThisKey : VectorCurve->YKeyInfo)
					{
						if (!ValidFKeyInfo(&ThisKey)) return false;
					}
				}
				if (VectorCurve->ZKeyInfo.Num() > 0)
				{
					for (FKeyInfo ThisKey : VectorCurve->ZKeyInfo)
					{
						if (!ValidFKeyInfo(&ThisKey)) return false;
					}
				}
				return true;
			}
		}
		#pragma endregion VectorCurve

		#pragma region
		if (const FLinearColorCurveInfo* LinearColorCurve = (FLinearColorCurveInfo*)(InCurve))
		{
			if (!LinearColorCurve->Guid.IsEmpty() && IsValid(LinearColorCurve->CurveObj))
			{
				if (LinearColorCurve->RKeyInfo.Num() > 0)
				{
					for (FKeyInfo ThisKey : LinearColorCurve->RKeyInfo)
					{
						if (!ValidFKeyInfo(&ThisKey)) return false;
					}
				}
				if (LinearColorCurve->GKeyInfo.Num() > 0)
				{
					for (FKeyInfo ThisKey : LinearColorCurve->GKeyInfo)
					{
						if (!ValidFKeyInfo(&ThisKey)) return false;
					}
				}
				if (LinearColorCurve->BKeyInfo.Num() > 0)
				{
					for (FKeyInfo ThisKey : LinearColorCurve->BKeyInfo)
					{
						if (!ValidFKeyInfo(&ThisKey)) return false;
					}
				}
				if (LinearColorCurve->AKeyInfo.Num() > 0)
				{
					for (FKeyInfo ThisKey : LinearColorCurve->AKeyInfo)
					{
						if (!ValidFKeyInfo(&ThisKey)) return false;
					}
				}
				return true;
			}
		}
		#pragma endregion LinearColorCurve
	}
	PrintError(LOCTEXT("CurveValidError", 
		"Please check whether the curve parameters and curve GUID and curve resources are correct."));
	return false;
}

// Finished TODO:Wait Valid
bool UAdvancedTimelineComponent::ValidFKeyInfo(FKeyInfo* InKey)
{
	/** 关键帧的值基本都是会默认赋值的，故不需要检查。 */
	if (InKey && !InKey->Guid.IsEmpty()) return true;

	PrintError(LOCTEXT("KeyValidError", 
		"Make sure that the GUID of the keyframe and itself have contents!"));
	return false;
	
}

// Finished
void UAdvancedTimelineComponent::GenerateDefaultTimeline(FTimelineInfo & OutTimeline, FTimelineSetting & OutSetting)
{
	FTimelineInfo DefaultTimeline;
	FTimelineSetting DefaultSetting;

	DefaultSetting.PlayRate = 1.0f;
	DefaultSetting.bIgnoreTimeDilation = false;
	DefaultSetting.bIsLoop = false;
	DefaultSetting.PlayMethod = EPlayMethod::Play;
	DefaultSetting.LengthMode = ELengthMode::Keyframe;
	DefaultSetting.Length = 0.0f;
	DefaultSetting.GuName = UKismetGuidLibrary::NewGuid().ToString();
	DefaultTimeline.GuName = UKismetGuidLibrary::NewGuid().ToString();
	DefaultTimeline.SettingGuName = DefaultSetting.GuName;
	DefaultTimeline.IsPlaying = false;
	DefaultTimeline.PlayStatus = EPlayStatus::Stop;
	DefaultTimeline.bReversePlayback = false;
	DefaultTimeline.Position = 0.0f;
	DefaultTimeline.IsStop = true;

	OutTimeline = DefaultTimeline;
	OutSetting = DefaultSetting;
}

// Finished
void UAdvancedTimelineComponent::PrintError()
{
	const FString ErrorInfo = FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(TEXT("There is an error here, please check him!"), TEXT(LOCTEXT_NAMESPACE), TEXT("GenericError")).ToString();
	UE_LOG(LogAdvTimeline, Error, TEXT("%s"), *ErrorInfo)
}

// Finished
void UAdvancedTimelineComponent::PrintError(const FString InFuncName)
{
	const FString ErrorInfo = LOCTEXT("FuncError", "function has an error!").ToString();
	UE_LOG(LogAdvTimeline, Error, TEXT("%s %s"), *InFuncName,*ErrorInfo)
}

// Finished
void UAdvancedTimelineComponent::PrintError(const FText InErrorInfo)
{
	const FString ErrorInfo = InErrorInfo.ToString();
	UE_LOG(LogAdvTimeline, Error, TEXT("%s"), *ErrorInfo)
}

// Finished
void UAdvancedTimelineComponent::PrintEmptyError()
{
	//GWorld->GetBlueprintObjectsBeingDebugged()
	//UBlueprint* bp=
	//FString StrToken = "AdvancedTimeline";
	const FString ErrorInfo = LOCTEXT("EmptyError", "The incoming parameter cannot be empty!").ToString();
	//CompilerLog->Error(*ErrorInfo, *StrToken);
	UE_LOG(LogAdvTimeline, Error, TEXT("%s"), *ErrorInfo)
}

#undef LOCTEXT_NAMESPACE