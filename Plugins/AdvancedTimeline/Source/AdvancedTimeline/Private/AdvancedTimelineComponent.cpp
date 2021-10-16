// Fill out your copyright notice in the Description page of Project Settings.

#include "AdvancedTimelineComponent.h"

#include "Culture.h"
#include "GameFramework/WorldSettings.h"
#include "Engine/World.h"
#include "Curves/CurveLinearColor.h"
#include "Curves/CurveVector.h"
#include "Engine/Engine.h"
#include "UObject/Package.h"
#include "Kismet/KismetGuidLibrary.h"


DEFINE_LOG_CATEGORY_STATIC(LogAdvTimeline, Log, All);

//本地化
#define LOCTEXT_NAMESPACE "AdvTimeline"
/** 本地化指南：
 *	只需要使用NSLOCTEXT()或者LOCTEXT()就可以了，剩下的操作需要进编辑器打开本地化Dash面板再改
 */

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::Activate(bool bReset)
{
	Super::Activate(bReset);
}

void UAdvancedTimelineComponent::Deactivate()
{
	Super::Deactivate();
}

bool UAdvancedTimelineComponent::IsReadyForOwnerToAutoDestroy() const
{
	return Super::IsReadyForOwnerToAutoDestroy();
}

void UAdvancedTimelineComponent::TickComponent(float DeltaTime, ELevelTick Tick,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, Tick, ThisTickFunction);
}

void UAdvancedTimelineComponent::GetTimelineInfo(const FString InTimelineGuid, FTimelineInfo& OutTimelineInfo)
{
}

void UAdvancedTimelineComponent::GetTimelineSettings(const FString InSettingName, const FString InSettingGuid,
	FTimelineSetting& OutTimelineSettings)
{
}

float UAdvancedTimelineComponent::GetPlaybackPosition(const FString InTrackGuid)
{
}

bool UAdvancedTimelineComponent::GetIgnoreTimeDilation(const FString InTimelineGuid)
{
}

float UAdvancedTimelineComponent::GetPlayRate(const FString InTrackGuid)
{
}

ELengthMode UAdvancedTimelineComponent::GetLengthMode(const FString InTrackGuid)
{
}

bool UAdvancedTimelineComponent::GetIsLoop(const FString InTrackGuid)
{
}

bool UAdvancedTimelineComponent::GetIsPause(const FString InTimelineGuid)
{
}

bool UAdvancedTimelineComponent::GetIsPlaying(const FString InTimelineGuid)
{
}

bool UAdvancedTimelineComponent::GetIsStop(const FString InTimelineGuid)
{
}

void UAdvancedTimelineComponent::GetAllFloatTracksInfo(const FString InTimelineGuid,
	TArray<FFloatTrackInfo>& OutFloatTracks)
{
}

void UAdvancedTimelineComponent::GetAllEventTracksInfo(const FString InTimelineGuid,
	TArray<FEventTrackInfo>& OutEventTracks)
{
}

void UAdvancedTimelineComponent::GetAllVectorTracksInfo(const FString InTimelineGuid,
	TArray<FVectorTrackInfo>& OutVectorTracks)
{
}

void UAdvancedTimelineComponent::GetAllLinearColorTracksInfo(const FString InTimelineGuid,
	TArray<FLinearColorTrackInfo>& OutLinearColorTracks)
{
}

int UAdvancedTimelineComponent::GetTrackCount(const FString InTimelineGuid)
{
}

float UAdvancedTimelineComponent::GetMinKeyframeTime(const FString InTimelineGuid)
{
}

float UAdvancedTimelineComponent::GetMaxKeyframeTime(const FString InTimelineGuid)
{
}

float UAdvancedTimelineComponent::GetMaxEndTime(const FString InTimelineGuid)
{
}

void UAdvancedTimelineComponent::AddUpdateCallback(const FString InTimelineGuid, const FOnTimelineEvent InUpdateEvent)
{
}

void UAdvancedTimelineComponent::AddFinishedCallback(const FString InTimelineGuid,
	const FOnTimelineEvent InFinishedEvent)
{
}

void UAdvancedTimelineComponent::AddTimelineInfo(const FTimelineInfo InTimeline,
	const FTimelineSetting InTimelineSetting)
{
}

void UAdvancedTimelineComponent::AddSettingToList(const FTimelineSetting InTimelineSetting)
{
}

void UAdvancedTimelineComponent::AddFloatTrack(const FString InTimelineGuid, const FFloatTrackInfo InFloatTrack)
{
}

void UAdvancedTimelineComponent::AddEventTrack(const FString InTimelineGuid, const FEventTrackInfo InEventTrack)
{
}

void UAdvancedTimelineComponent::AddVectorTrack(const FString InTimelineGuid, const FVectorTrackInfo InVectorTrack)
{
}

void UAdvancedTimelineComponent::AddLinearColorTrack(const FString InTimelineGuid,
	const FLinearColorTrackInfo InLinearColorTrack)
{
}

void UAdvancedTimelineComponent::GenericAddTrack(const FString InTimelineGuid, const EAdvTimelineType InTrackType,
	const UScriptStruct* StructType, const void* InTrack)
{
}

void UAdvancedTimelineComponent::SetLength(const FString InTrackGuid, const float InNewTime)
{
}

void UAdvancedTimelineComponent::SetIgnoreTimeDilation(const FString InTimelineGuid, const bool IsIgnoreTimeDilation)
{
}

void UAdvancedTimelineComponent::SetPlaybackPosition(const FString InTrackGuid, const float InNewTime,
	const bool bIsFireEvent, const bool bIsFireUpdate, const bool bIsFirePlay)
{
}

void UAdvancedTimelineComponent::SetTrackPlayRate(const FString InTrackGuid, const float InNewPlayRate)
{
}

void UAdvancedTimelineComponent::SetTrackTimeMode(const FString InTrackGuid, const ELengthMode InEndTimeMode)
{
}

void UAdvancedTimelineComponent::SetTimelineLoop(const FString InTimelineGuid, const bool bIsLoop)
{
}

void UAdvancedTimelineComponent::SetTrackLoop(const FString InTrackGuid, const bool bIsLoop)
{
}

void UAdvancedTimelineComponent::SetTimelineAutoPlay(const FString InTimelineGuid, const bool bIsAutoPlay)
{
}

void UAdvancedTimelineComponent::SetTrackAutoPlay(const FString InTrackGuid, const bool bIsAutoPlay)
{
}

void UAdvancedTimelineComponent::SetTimelinePlayRate(const FString InTimelineGuid, const float InNewPlayRate)
{
}

void UAdvancedTimelineComponent::ChangeTrackCurve(const FString InTrackGuid, const UCurveBase* InCurve)
{
}

void UAdvancedTimelineComponent::ChangeTimelineName(const FString InTimelineGuid, const FText InNewName)
{
}

void UAdvancedTimelineComponent::ChangeSettingName(const FString InSettingGuid, const FText InNewName)
{
}

void UAdvancedTimelineComponent::ChangeTrackName(const FString InTrackGuid, const FString InNewName)
{
}

void UAdvancedTimelineComponent::Play(const FString InTimelineGuid, const EPlayMethod PlayMethod)
{
}

void UAdvancedTimelineComponent::Stop(const FString InTimelineGuid, const bool bIsPause)
{
}

void UAdvancedTimelineComponent::ResetTimeline(const FString InTimelineGuid)
{
}

void UAdvancedTimelineComponent::ClearTimelineTracks(const FString InTimelineGuid)
{
}

void UAdvancedTimelineComponent::ResetTrack(const FString InTrackGuid)
{
}

void UAdvancedTimelineComponent::DelTrack(const FString InTrackGuid)
{
}

void UAdvancedTimelineComponent::ApplySettingToTimeline(const FString InSettingsGuid, const FString InTimelineGuid,
	FString& OutOriginSettingsGuid)
{
}

void UAdvancedTimelineComponent::DelTimeline(const FString InTimelineGuid)
{
}

void UAdvancedTimelineComponent::ClearTimelines()
{
}

void UAdvancedTimelineComponent::ClearSettingList()
{
}

void UAdvancedTimelineComponent::TickTimeline(const FString InTimelineGuid, float DeltaTime)
{
}

bool UAdvancedTimelineComponent::QueryTimelineByGuid(const FString InTimelineGuid, FTimelineInfo& OutTimelineInfo)
{
}

bool UAdvancedTimelineComponent::QuerySettingsByGuid(const FString InSettingsGuid, FTimelineSetting& OutTimelineSetting,
	FTimelineInfo& OutTimeline)
{
}

bool UAdvancedTimelineComponent::QueryTrackByGuid(const FString InTrackGuid, EAdvTimelineType& OutTrackType,
	FTrackInfoBase*& OutTrack, FTimelineInfo& OutTimeline)
{
}

bool UAdvancedTimelineComponent::QueryCurveByGuid(const FString InCurveGuid, EAdvTimelineType& OutCurveType,
	FCurveInfoBase*& OutCurve, FTimelineInfo& OutTimeline)
{
}

bool UAdvancedTimelineComponent::QueryKeyByGuid(const FString InKeyGuid, FString& OutTrackGuid, FKeyInfo*& OutKey,
	FTimelineInfo& OutTimeline)
{
}

bool UAdvancedTimelineComponent::ValidFTimelineInfo(const FTimelineInfo* InTimelineInfo)
{
}

bool UAdvancedTimelineComponent::ValidFTimelineSetting(const FTimelineSetting* InSettingsInfo)
{
}

bool UAdvancedTimelineComponent::ValidFTrackInfo(const FTrackInfoBase* InTrack)
{
}

bool UAdvancedTimelineComponent::ValidFCurveInfo(const FCurveInfoBase* InCurve)
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
				if (EventCurve->CurveKeyInfo.Num() > 0)
				{
					for (FKeyInfo ThisKey : EventCurve->CurveKeyInfo)
					{
						if (!ValidFKeyInfo(&ThisKey)) return false;
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
				if (VectorCurve->CurveKeyInfo.Num() > 0)
				{
					for (FKeyInfo ThisKey : VectorCurve->CurveKeyInfo)
					{
						if (!ValidFKeyInfo(&ThisKey)) return false;
					}
				}
				return true;
			}
		}
		#pragma endregion VectorCurve

		#pragma region
		if (const FLinearColorCurveInfo*  LinearColorCurve = (FLinearColorCurveInfo*)(InCurve))
		{
			if (!LinearColorCurve->Guid.IsEmpty() && IsValid(LinearColorCurve->CurveObj))
			{
				if (LinearColorCurve->CurveKeyInfo.Num() > 0)
				{
					for (FKeyInfo ThisKey : LinearColorCurve->CurveKeyInfo)
					{
						if (!ValidFKeyInfo(&ThisKey)) return false;
					}
				}
				return true;
			}
		}
		#pragma endregion LinearColorCurve
	}
	return false;
}

// Finished TODO:Wait Valid
bool UAdvancedTimelineComponent::ValidFKeyInfo(FKeyInfo* InKey)
{
	/** 关键帧的值基本都是会默认赋值的，故不需要检查。 */
	if (InKey && !InKey->Guid.IsEmpty()) return true;

	PrintError("KeyValidError", "Make sure that the GUID of the keyframe and itself have contents!");
	return false;
	
}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::GenerateDefaultTimeline(FTimelineInfo& OutTimeline,
	FTimelineSetting& OutTimelineSetting)
{
	FTimelineInfo DefaultTimeline;
	FTimelineSetting DefaultSetting;

	DefaultSetting.Guid = UKismetGuidLibrary::NewGuid().ToString();
	DefaultSetting.PlayRate = 1.0f;
	DefaultSetting.bIgnoreTimeDilation = false;
	DefaultSetting.bIsLoop = false;
	DefaultSetting.PlayMethod = EPlayMethod::Play;
	DefaultSetting.LengthMode = ELengthMode::Keyframe;
	DefaultTimeline.Guid = UKismetGuidLibrary::NewGuid().ToString();
	DefaultTimeline.TimelineSetting = DefaultSetting;

	DefaultTimeline.TimelineName = LOCTEXT("DefaultTimeline", "DefaultTimeline");
	DefaultSetting.SettingName = LOCTEXT("DefaultSetting", "DefaultSetting");

	OutTimeline = DefaultTimeline;
	OutTimelineSetting = DefaultSetting;

}

// Finished TODO:Wait Valid
void UAdvancedTimelineComponent::PrintError(const FString InKey, const FString InText)
{
	const FString ErrorInfo = FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(*InText, TEXT(LOCTEXT_NAMESPACE), *InKey).ToString();
	UE_LOG(LogAdvTimeline, Error, TEXT("%s"), *ErrorInfo)
}

#undef LOCTEXT_NAMESPACE
