// Fill out your copyright notice in the Description page of Project Settings.

// ReSharper disable CppExpressionWithoutSideEffects
// ReSharper disable CppClangTidyPerformanceUnnecessaryValueParam
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

/** 播放的时候执行这个 */
void UAdvancedTimelineComponent::Activate(bool bReset)
{
	Super::Activate(bReset);
	PrimaryComponentTick.SetTickFunctionEnable(true);
}

/** 停止的时候执行这个 */
void UAdvancedTimelineComponent::Deactivate()
{
	Super::Deactivate();
	PrimaryComponentTick.SetTickFunctionEnable(false);
}

/** 不知道干嘛用的，放着吧 */
bool UAdvancedTimelineComponent::IsReadyForOwnerToAutoDestroy() const
{
	return !PrimaryComponentTick.IsTickFunctionEnabled();
}

/** 主要函数之一 */
void UAdvancedTimelineComponent::TickComponent(float DeltaTime, ELevelTick Tick,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, Tick, ThisTickFunction);

	// 如果存在时间线
	if (Timelines.Num() > 0)
	{
		// 循环一下
		for (TPair<FString, FTimelineInfo> ThisTimeline : Timelines)
		{
			// 如果这个时间线有问题，跳过他
			if (!ValidFTimelineInfo(&ThisTimeline.Value)) continue;
			FTimelineSetting* ThisSetting;
			// 如果没有找到这个配置或者他有问题，跳过他
			if (!QuerySetting(ThisTimeline.Value.SettingGuName, ThisSetting) && !ValidFTimelineSetting(ThisSetting)) continue;

			// 如果需要忽略时间膨胀
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
			// 主要函数之二
			TickTimeline(ThisTimeline.Value.GuName, DeltaTime);

			// 好像是网络复制同步用的
			if (!IsNetSimulating())
			{
				EPlayStatus PlayStatus = GetPlayStatus(ThisTimeline.Value.GuName);
				// Do not deactivate if we are done, since bActive is a replicated property and we shouldn't have simulating
				// clients touch replicated variables.
				// 如果我们完成了，就不要停用，因为bActive是一个复制的属性，我们不应该让模拟客户接触复制的变量。
				if (!(PlayStatus == EPlayStatus::ReversePlaying || PlayStatus == EPlayStatus::ForwardPlaying))
				{
					Deactivate();
				}
			}
		}
	}
}

/** 主要函数之二 */
void UAdvancedTimelineComponent::TickTimeline(const FString InTimelineGuName, float DeltaTime)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}

	FTimelineInfo* ThisTimeline;
	QueryTimeline(InTimelineGuName, ThisTimeline);

	// Tick之前先确认一下Timeline的有效性，虽然前面有确认，但是为了保险应该每次使用Timeline之前确认一下
	if (ValidFTimelineInfo(ThisTimeline))
	{
		FTimelineSetting* ThisSetting;
		QuerySetting(ThisTimeline->SettingGuName, ThisSetting);
		// 配置也得确认一下，因为时间线部分属性存在配置内
		if (ValidFTimelineSetting(ThisSetting)) {
			EPlayStatus playStatus = GetPlayStatus(ThisTimeline->GuName);
			bool bIsFinished = false;

			// 如果正在播放
			if (playStatus == EPlayStatus::ReversePlaying || playStatus == EPlayStatus::ForwardPlaying)
			{
				// 这个是当前位置
				const float TimelineLength = GetTimelineLength(ThisTimeline->GuName);
				// 这个↓是真实使用的Time，因为考虑到了是否反转和播放速度。
				const float EffectiveDeltaTime = DeltaTime * (playStatus == EPlayStatus::ReversePlaying ? (-ThisSetting->PlayRate) : (ThisSetting->PlayRate));

				// 这个是下一帧位置
				float NewPosition = ThisTimeline->Position + EffectiveDeltaTime;

				// 没有=0，因为播放速度为0就是停止了
				// > 0 正向播放
				if (EffectiveDeltaTime > 0.0f)
				{
					// 如果播放到了末尾
					if (NewPosition > TimelineLength)
					{
						// 如果需要循环，跳转到开头。记得先跳转到结尾一次，放止播放超出时间线
						if (ThisSetting->bIsLoop)
						{
							SetPlaybackPosition(ThisTimeline->GuName, TimelineLength, true);
							SetPlaybackPosition(ThisTimeline->GuName, 0.f, false);

							// 确保下一帧位置在正确的值上。如果是正向播放就-长度，放止播放到了奇怪的位置
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
						// 如果不循环，就停止播放
						else
						{
							NewPosition = TimelineLength;
							Stop(ThisTimeline->GuName, true);
							bIsFinished = true;
						}
					}
				}
				// < 0反向播放
				else
				{
					if (NewPosition < 0.f)
					{
						// 因为是反向播放，所以结尾在0
						if (ThisSetting->bIsLoop)
						{
							SetPlaybackPosition(ThisTimeline->GuName, 0.f, true);
							SetPlaybackPosition(ThisTimeline->GuName, TimelineLength, false);

							// 确保下一帧位置在正确的值上。如果是反向播放就+长度，放止播放到了奇怪的位置
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
						// 不循环，停止播放
						else
						{
							NewPosition = 0.f;
							Stop(ThisTimeline->GuName);
							bIsFinished = true;
						}
					}
				}
				// 跳转到下一帧位置
				SetPlaybackPosition(ThisTimeline->GuName, NewPosition, true);
			}

			// 如果完成了播放，就通知时间线执行完成函数，这里调用函数用的是多播委托来通知到目标函数
			if (bIsFinished)
			{
				ThisTimeline->PlayStatus = EPlayStatus::Stop;
				AdvTimelineFinishedDelegate.Broadcast(ThisSetting->FinishedEventGuName);
			}
		}
	}// 如果时间线或者配置没有通过验证，就报个错
	else PrintError(LOCTEXT("TickTimelineError", "Tick timeline with errors! Please check if the required parameters are filled in the timeline!"));
}

/** 播放 */
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
			// 调用函数，开始Tick
			Activate();

			// 设置播放状态
			if (PlayMethod == EPlayMethod::PlayOnStart)
			{
				SetPlaybackPosition(ThisTimeline->GuName, 0.f, true);
				ThisTimeline->PlayStatus = EPlayStatus::ForwardPlaying;
			}
			else if (PlayMethod == EPlayMethod::Reverse)
			{
				ThisTimeline->PlayStatus = EPlayStatus::ReversePlaying;
			}
			else if (PlayMethod == EPlayMethod::ReverseOnEnd)
			{
				SetPlaybackPosition(ThisTimeline->GuName, ThisSetting->Length, true);
				ThisTimeline->PlayStatus = EPlayStatus::ReversePlaying;
			}
			else if (PlayMethod == EPlayMethod::Play)
			{
				ThisTimeline->PlayStatus = EPlayStatus::ForwardPlaying;
			}
			return;
		}
	}
	PrintError(LOCTEXT("PlayError",
		"There is an error in the timeline that needs to be played, please check if the required parameters are filled in!"));
}

/** 停止播放 */
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
				ThisTimeline->PlayStatus = EPlayStatus::Stop;
				// 这里不能执行这个函数，要交给Tick去判断，因为需要调用Finished事件
				// Deactivate();
			}
			else
			{
				ThisTimeline->PlayStatus = EPlayStatus::Pause;
			}
			return;
		}
	}
	PrintError("Stop");
}

/** 获取当前播放位置 */
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

/** 获取是否忽略时间膨胀 */
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
	}
	PrintError("GetIgnoreTimeDilation");
	return false;
}

/** 获取播放速度 */
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
	}
	PrintError("GetPlayRate");
	return false;
}

/** 获取时间线长度模式 */
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
	}
	PrintError("GetLengthMode");
	return ELengthMode::Keyframe;
}

/** 获取是否循环 */
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

/** 获取是否暂停 */
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
		if (GetPlayStatus(InTimelineGuName) == EPlayStatus::Pause)
			return true;
		return false;
	}

	PrintError("GetIsPause");
	return false;
}

/** 获取是否正在播放 */
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
		EPlayStatus playStatus = GetPlayStatus(InTimelineGuName);
		if (playStatus == EPlayStatus::ForwardPlaying || playStatus == EPlayStatus::ReversePlaying)
			return true;
		return false;
	}

	PrintError("GetIsPlaying");
	return false;
}

/** 获取是否停止了播放 */
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
		if (GetPlayStatus(InTimelineGuName) == EPlayStatus::Stop)
			return true;
		return false;
	}

	PrintError("GetIsStop");
	return false;
}

/** 获取全部浮点轨道的信息 */
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

/** 获取全部事件轨道的信息 */
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

/** 获取全部向量轨道的信息 */
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

/** 获取全部颜色轨道的信息 */
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

/** 获取轨道总数量 */
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

/** 获取最小关键帧时间 */
float UAdvancedTimelineComponent::GetMinKeyframeTime(const FString InTimelineGuName)
{
	// 先给个极大值，用来判断是否是最小关键帧
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
			// 循环TMap要用TPair来ForEach
			for (TPair < FString, FEventTrackInfo> ThisTrack : ThisTimeline->EventTracks)
			{
				if (ValidFTrackInfo(&ThisTrack.Value) && ThisTrack.Value.CurveInfo.EventKey.Num() > 0)
				{
					// 直接拿到第一个关键帧的时间来判断哪个更小就可以了。很简单的算法，有更好的方案欢迎评论
					if (ThisTrack.Value.CurveInfo.CurveObj->FloatCurve.GetFirstKey().Time < MinKeyTime)
						MinKeyTime = ThisTrack.Value.CurveInfo.CurveObj->FloatCurve.GetFirstKey().Time;
				}
			}
		}
		#pragma endregion EventTrack

		#pragma region
		if (ThisTimeline->FloatTracks.Num() > 0)
		{
			// 循环TMap要用TPair来ForEach
			for (TPair < FString, FFloatTrackInfo> ThisTrack : ThisTimeline->FloatTracks)
			{
				if (ValidFTrackInfo(&ThisTrack.Value) && ThisTrack.Value.CurveInfo.CurveKeyInfo.Num() > 0)
				{
					// 直接拿到第一个关键帧的时间来判断哪个更小就可以了。很简单的算法，有更好的方案欢迎评论
					if (ThisTrack.Value.CurveInfo.CurveObj->FloatCurve.GetFirstKey().Time < MinKeyTime)
						MinKeyTime = ThisTrack.Value.CurveInfo.CurveObj->FloatCurve.GetFirstKey().Time;
				}
			}
		}
		#pragma endregion FloatTrack

		#pragma region
		if (ThisTimeline->VectorTracks.Num() > 0)
		{
			// 循环TMap要用TPair来ForEach
			for (TPair < FString, FVectorTrackInfo> ThisTrack : ThisTimeline->VectorTracks)
			{
				if (ValidFTrackInfo(&ThisTrack.Value)) {
					// 直接拿到第一个关键帧的时间来判断哪个更小就可以了。很简单的算法，有更好的方案欢迎评论
					// 向量由三个Float曲线构成，每个都得循环，因为有可能某个曲线的时间更小
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
			// 循环TMap要用TPair来ForEach
			for (TPair < FString, FLinearColorTrackInfo> ThisTrack : ThisTimeline->LinearColorTracks)
			{
				if (ValidFTrackInfo(&ThisTrack.Value)) {
					// 直接拿到第一个关键帧的时间来判断哪个更小就可以了。很简单的算法，有更好的方案欢迎评论
					// 颜色由四个Float曲线构成，每个都得循环，因为有可能某个曲线的时间更小
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

/** 获取最大关键帧时间 */
float UAdvancedTimelineComponent::GetMaxKeyframeTime(const FString InTimelineGuName)
{
	// 此处函数逻辑同上个函数，反一下就可以了
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
			for (TPair<FString, FEventTrackInfo> ThisTrack : ThisTimeline->EventTracks)
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

/** 获取时间线长度 */
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
			// 注意这里不能直接返回长度，要考虑到长度模式。
			switch (ThisSetting->LengthMode)
			{
				case ELengthMode::Custom:
					return ThisSetting->Length;
				case ELengthMode::Keyframe:
					return GetMaxKeyframeTime(InTimelineGuName);
			}
		}
		PrintError("GetTimelineLength");
		return 0.0f;
	}
	PrintError("GetTimelineLength");
	return 0.0f;
}

/** 获取当前播放状态，当然直接break结构体也可以 */
EPlayStatus UAdvancedTimelineComponent::GetPlayStatus(const FString InTimelineGuName)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return EPlayStatus::Stop;
	}

	FTimelineInfo* ThisTimeline;
	if (QueryTimeline(InTimelineGuName, ThisTimeline))
	{
		return ThisTimeline->PlayStatus;
	}
	PrintError("GetPlayStatus");
	return EPlayStatus::Stop;
}

/** 添加Update事件，反复添加会覆盖。后续看是否需要改成TArray */
void UAdvancedTimelineComponent::AddUpdateCallback(const FString InTimelineGuName, const FString InUpdateEventGuName)
{
	if (InTimelineGuName.IsEmpty() || InUpdateEventGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}

	FTimelineInfo* ThisTimeline;
	if (QueryTimeline(InTimelineGuName, ThisTimeline)) {
		FTimelineSetting* ThisSetting;
		if (QuerySetting(ThisTimeline->SettingGuName, ThisSetting)) {
			ThisSetting->UpdateEventGuName = InUpdateEventGuName;
			return;
		}
		PrintError("AddUpdateCallback");
		return;
	}
	PrintError("AddUpdateCallback");
}

/** 添加Finished事件，反复添加会覆盖。后续看是否需要改成TArray */
void UAdvancedTimelineComponent::AddFinishedCallback(const FString InTimelineGuName, const FString InFinishedEventGuName)
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
			ThisSetting->FinishedEventGuName = InFinishedEventGuName;
			return;
		}
		PrintError("AddFinishedCallback");
		return;
	}
	PrintError("AddFinishedCallback");
}

/** 添加一个时间线 */
void UAdvancedTimelineComponent::AddTimelineInfo(const FTimelineInfo InTimeline, const FTimelineSetting InSetting)
{
	if (!ValidFTimelineInfo(&InTimeline))
	{
		PrintError(LOCTEXT("AddTimelineInfoError",
			"There is an error in the TimelineInfo passed in, please check if you filled in the required fields!"));
		return;
	}
	Timelines.Add(InTimeline.GuName, InTimeline);
	SettingList.Add(InTimeline.SettingGuName, InSetting);
}

/** 保存一个配置，这个配置不会应用到时间线上，需要手动执行ApplySettingToTimeline函数 */
void UAdvancedTimelineComponent::SaveSetting(const FTimelineSetting InTimelineSetting)
{
	if (ValidFTimelineSetting(&InTimelineSetting))
	{
		PrintError(LOCTEXT("SaveSettingError",
			"There is an error in the TimelineSetting passed in, please check if you filled in the required fields!"));
		return;
	}
	SettingList.Add(InTimelineSetting.GuName, InTimelineSetting);
}

/** 添加一个浮点轨道 */
void UAdvancedTimelineComponent::AddFloatTrack(const FString InTimelineGuName, FFloatTrackInfo InFloatTrack)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}
	if (!ValidFTrackInfo(&InFloatTrack))
	{
		PrintError(LOCTEXT("AddFloatTrackError",
			"There is an error in the FloatTrack passed in, please check if you filled in the required fields!"));
		return;
	}

	if (UCurveFloat* ThisFloatCurve = InFloatTrack.CurveInfo.CurveObj)
	{
		// 如果存在关键帧，就保存一下关键帧，方便后续更改
		if (ThisFloatCurve->FloatCurve.GetNumKeys() > 0)
		{
			for (auto It(ThisFloatCurve->FloatCurve.GetKeyIterator()); It; ++It)
			{
				FKeyInfo ThisFloatKey;
				ThisFloatKey.BaseKey = *It;
				ThisFloatKey.InterpMode = FKeyInfo::ConversionKeyModeToInterpMode(*It);
				InFloatTrack.CurveInfo.CurveKeyInfo.Add(ThisFloatKey);
			}
		}
	}
	FTimelineInfo* ThisTimeline;

	if (QueryTimeline(InTimelineGuName, ThisTimeline)) {
		ThisTimeline->FloatTracks.Add(InFloatTrack.GuName, InFloatTrack);
		return;
	}
	PrintError("AddFloatTrack");
}

/** 添加一个事件轨道 */
void UAdvancedTimelineComponent::AddEventTrack(const FString InTimelineGuName, FEventTrackInfo InEventTrack)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}

	if (!ValidFTrackInfo(&InEventTrack))
	{
		PrintError(LOCTEXT("AddEventTrackError",
			"There is an error in the EventTrack passed in, please check if you filled in the required fields!"));
		return;
	}

	if (UCurveFloat* ThisEventCurve = InEventTrack.CurveInfo.CurveObj)
	{
		if (ThisEventCurve->FloatCurve.GetNumKeys() > 0)
		{
			for (auto It(ThisEventCurve->FloatCurve.GetKeyIterator()); It; ++It)
			{
				FEventKey ThisEventKey;
				ThisEventKey.BaseKey = *It;
				ThisEventKey.Time = It->Time;
				InEventTrack.CurveInfo.EventKey.Add(ThisEventKey);
			}
		}
	}

	FTimelineInfo* ThisTimeline;

	if (QueryTimeline(InTimelineGuName, ThisTimeline)) {
		ThisTimeline->EventTracks.Add(InEventTrack.GuName, InEventTrack);
		return;
	}
	PrintError("AddEventTrack");
}

/** 添加一个向量轨道 */
void UAdvancedTimelineComponent::AddVectorTrack(const FString InTimelineGuName, FVectorTrackInfo InVectorTrack)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}
	if (!ValidFTrackInfo(&InVectorTrack))
	{
		PrintError(LOCTEXT("AddVectorTrackError",
			"There is an error in the VectorTrack passed in, please check if you filled in the required fields!"));
		return;
	}

	if (UCurveVector* ThisVectorCurve = InVectorTrack.CurveInfo.CurveObj)
	{
		for (int i = 0; i < 3; i++)
		{
			if (ThisVectorCurve->FloatCurves[i].GetNumKeys() > 0)
			{
				for (auto It(ThisVectorCurve->FloatCurves[i].GetKeyIterator()); It; ++It)
				{
					FKeyInfo ThisVectorKey;
					ThisVectorKey.BaseKey = *It;
					ThisVectorKey.InterpMode = FKeyInfo::ConversionKeyModeToInterpMode(*It);
					// X,Y,Z按照顺序添加就行了，官方的应该也是按照顺序来的，不至于乱序
					if (i == 0) InVectorTrack.CurveInfo.XKeyInfo.Add(ThisVectorKey);
					else if (i == 1) InVectorTrack.CurveInfo.YKeyInfo.Add(ThisVectorKey);
					else if (i == 2) InVectorTrack.CurveInfo.ZKeyInfo.Add(ThisVectorKey);
				}
			}
		}
	}

	FTimelineInfo* ThisTimeline;

	if (QueryTimeline(InTimelineGuName, ThisTimeline)) {
		ThisTimeline->VectorTracks.Add(InVectorTrack.GuName, InVectorTrack);
		return;
	}
	PrintError("AddVectorTrack");
}

/** 添加一个颜色轨道 */
void UAdvancedTimelineComponent::AddLinearColorTrack(const FString InTimelineGuName, FLinearColorTrackInfo InLinearColorTrack)
{
	if (InTimelineGuName.IsEmpty())
	{
		PrintEmptyError();
		return;
	}
	if (!ValidFTrackInfo(&InLinearColorTrack))
	{
		PrintError(LOCTEXT("AddLinearColorTrackError",
			"There is an error in the LinearColorTrack passed in, please check if you filled in the required fields!"));
		return;
	}

	if (UCurveLinearColor* ThisLinearColorCurve = InLinearColorTrack.CurveInfo.CurveObj)
	{

		for (int i = 0; i < 4; i++)
		{
			if (ThisLinearColorCurve->FloatCurves[i].GetNumKeys() > 0)
			{
				for (auto It(ThisLinearColorCurve->FloatCurves[i].GetKeyIterator()); It; ++It)
				{
					FKeyInfo ThisLinearColorKey;
					ThisLinearColorKey.BaseKey = *It;
					ThisLinearColorKey.InterpMode = FKeyInfo::ConversionKeyModeToInterpMode(*It);
					if (i == 0) InLinearColorTrack.CurveInfo.RKeyInfo.Add(ThisLinearColorKey);
					else if (i == 1) InLinearColorTrack.CurveInfo.GKeyInfo.Add(ThisLinearColorKey);
					else if (i == 2) InLinearColorTrack.CurveInfo.BKeyInfo.Add(ThisLinearColorKey);
					else if (i == 3) InLinearColorTrack.CurveInfo.AKeyInfo.Add(ThisLinearColorKey);
				}
			}
		}
	}

	FTimelineInfo* ThisTimeline;

	if (QueryTimeline(InTimelineGuName, ThisTimeline)) {
		ThisTimeline->LinearColorTracks.Add(InLinearColorTrack.GuName, InLinearColorTrack);
		return;
	}
	PrintError("AddLinearColorTrack");
}

/** 通用的添加轨道 */
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
		if (Cast<UStruct>(StructType) == FFloatTrackInfo::StaticStruct())// 判断结构体类型是不是需要的那个
		{
			FFloatTrackInfo* Float = (FFloatTrackInfo*)(InTrack);// 这里转成需要的结构体直接这样转就行，因为是指针类型的。UE的Cast不能用，会报错
			if (!ValidFTrackInfo(Float))
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
			if (!ValidFTrackInfo(Event))
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
			if (!ValidFTrackInfo(Vector))
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
			if (!ValidFTrackInfo(LinearColor))
			{
				PrintError(LOCTEXT("GenericAddTrackError1",
					"There is an error in the incoming FloatTrack, \
								please make sure that the track type is correct and that the required parameters are indeed filled in!"));
				return;
			}
			AddLinearColorTrack(InTimelineGuName, *LinearColor);
		}
		#pragma endregion LinearColorTrack
		return;
	}
	PrintError("GenericAddTrack");
}

/** 设置时间线的长度，决定了时间线播放多久 */
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
			return;
		}
		PrintError("SetLength");
		return;
	}
	PrintError("SetLength");
}

/** 设置是否忽略时间膨胀 */
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
			return;
		}
		PrintError("SetIgnoreTimeDilation");
		return;
	}
	PrintError("SetIgnoreTimeDilation");
}

/** 主要函数之三 */
void UAdvancedTimelineComponent::SetPlaybackPosition(
	const FString InTimelineGuName,
	const float InNewTime,
	const bool bIsFireEvent,
	const bool bIsFireUpdate  /** = true */)
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
		EPlayStatus playStatus = GetPlayStatus(ThisTimeline->GuName);
		const float OldPosition = ThisTimeline->Position;
		/** 记得把新的播放位置设过去
		 *	这里因为SetPlaybackPosition就是自身，所以直接设置就可以了
		 */
		ThisTimeline->Position = InNewTime;

		// 遍历向量轨道
		for (const TPair<FString, FVectorTrackInfo> CurrentEntry : ThisTimeline->VectorTracks)
		{
			if (CurrentEntry.Value.CurveInfo.CurveObj && !CurrentEntry.Value.EventFuncGuName.IsEmpty())
			{
				// 获得当前跳转过去后的那个时间点上的值
				// 这里用InNewTime，ThisTimeline->Position，GetPlaybackPosition()都可以
				FVector const Vec = CurrentEntry.Value.CurveInfo.CurveObj->GetVectorValue(InNewTime);

				// 多播出去执行函数
				AdvTimelineVectorDelegate.Broadcast(CurrentEntry.Value.EventFuncGuName, Vec);
			}
		}

		// 遍历浮点轨道
		for (const TPair < FString, FFloatTrackInfo> CurrentEntry : ThisTimeline->FloatTracks)
		{
			if (CurrentEntry.Value.CurveInfo.CurveObj && !CurrentEntry.Value.EventFuncGuName.IsEmpty())
			{
				// Get float from func
				const float Val = CurrentEntry.Value.CurveInfo.CurveObj->GetFloatValue(InNewTime);

				// Pass float to specified function
				AdvTimelineFloatDelegate.Broadcast(CurrentEntry.Value.EventFuncGuName, Val);
			}
		}

		// 遍历颜色轨道
		for (const TPair < FString, FLinearColorTrackInfo> CurrentEntry : ThisTimeline->LinearColorTracks)
		{
			if (CurrentEntry.Value.CurveInfo.CurveObj && !CurrentEntry.Value.EventFuncGuName.IsEmpty())
			{
				// Get vector from curve
				const FLinearColor Color = CurrentEntry.Value.CurveInfo.CurveObj->GetLinearColorValue(InNewTime);

				// Pass vec to specified function
				AdvTimelineLinearColorDelegate.Broadcast(CurrentEntry.Value.EventFuncGuName, Color);
			}
		}


		// 如果需要触发事件
		if (bIsFireEvent)
		{
			// If playing sequence forwards.
			float MinTime, MaxTime;
			if (playStatus == EPlayStatus::ForwardPlaying)// 如果是正向播放
			{
				MinTime = OldPosition;
				MaxTime = ThisTimeline->Position;

				// 如果正向播放并到达序列的末尾，将它偏移一下，以确保我们在序列的末尾能够正确发射事件。
				if (MaxTime == GetTimelineLength(ThisTimeline->GuName))
				{
					MaxTime += KINDA_SMALL_NUMBER;
				}
			}
			// 如果是反向播放
			else if (playStatus == EPlayStatus::ReversePlaying)
			{
				MinTime = ThisTimeline->Position;
				MaxTime = OldPosition;

				// 原因同上，但是因为是反向播放，所以-
				if (MinTime == 0.f)
				{
					MinTime -= KINDA_SMALL_NUMBER;
				}
			}

			// 遍历所有的事件轨道
			for (const TPair < FString, FEventTrackInfo> ThisEventTrack : ThisTimeline->EventTracks)
			{
				// 遍历当前轨道的所有关键帧，这里没有循环保存的关键帧，是为了确保正确所以用了官方的，之后可以考虑使用自定义的
				for (auto It(ThisEventTrack.Value.CurveInfo.CurveObj->FloatCurve.GetKeyIterator()); It; ++It)
				{
					float EventTime = It->Time;

					// 这里需要稍加注意，在向前和向后播放时，要使触发事件的行为对称。
					bool bFireThisEvent = false;
					if (playStatus == EPlayStatus::ForwardPlaying)// 如果是正向播放
					{
						// 实际上两边都加=可能会好点？
						if (EventTime >= MinTime && EventTime < MaxTime)
						{
							bFireThisEvent = true;
						}
					}
					// 如果是反向播放
					else if (playStatus == EPlayStatus::ReversePlaying)
					{
						// 实际上两边都加=可能会好点？
						if (EventTime > MinTime && EventTime <= MaxTime)
						{
							bFireThisEvent = true;
						}
					}

					if (bFireThisEvent)
					{
						// 如果确实需要触发事件，因为播放状态还可能是暂停和停止，但是却进到了这里。算是一个保险吧
						AdvTimelineEventDelegate.Broadcast(ThisEventTrack.Value.EventFuncGuName);
					}
				}
			}
		}
		// 如果需要触发Update事件
		if (bIsFireUpdate)
		{
			FTimelineSetting* ThisSetting;

			if (QuerySetting(ThisTimeline->SettingGuName, ThisSetting)) {
				AdvTimelineUpdateDelegate.Broadcast(ThisSetting->UpdateEventGuName);
				return;
			}
			// 因为后面没有其他逻辑了，所以直接Return就行，不然会输出两遍Error
			PrintError("SetPlaybackPosition");
			return;
		}
	}
	PrintError("SetPlaybackPosition");
}

/** 设置时间线的长度模式 */
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
			return;
		}
		PrintError("SetLengthMode");
		return;
	}
	PrintError("SetLengthMode");
}

/** 设置是否循环 */
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
			return;
		}
		PrintError("SetLoop");
		return;
	}
	PrintError("SetLoop");
}

/** 设置播放速度 */
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
			return;
		}
		PrintError("SetPlayRate");
		return;
	}
	PrintError("SetPlayRate");
}

/** 更改轨道的曲线 */
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
				if (FFloatTrackInfo* FloatTrack = (FFloatTrackInfo*)(BaseTrack))// 因为查询到后返回的轨道信息是基类的，所以需要先转换一下(实际的地址是子类的)
				{
					if (ValidFTrackInfo(FloatTrack))
					{
						FloatTrack->CurveInfo.CurveObj = Cast<UCurveFloat>(InCurve);// 因为传入的新曲线也是基类，所以也要转换一下，这里因为是UE内置的类型，所以可以用Cast
						FloatTrack->CurveInfo.CurveType = EAdvTimelineType::Float;
						// 因为更换曲线之前有可能已经存在了关键帧数据，所以为了避免之后查找等错误，先清空一下
						FloatTrack->CurveInfo.CurveKeyInfo.Empty();
						if (FloatTrack->CurveInfo.CurveObj->FloatCurve.GetNumKeys() > 0)
						{
							// 如果存在关键帧，就保存一下
							for (auto It(FloatTrack->CurveInfo.CurveObj->FloatCurve.GetKeyIterator()); It; ++It)
							{
								FKeyInfo ThisKey;
								ThisKey.Guid = UKismetGuidLibrary::NewGuid().ToString();
								ThisKey.InterpMode = FKeyInfo::ConversionKeyModeToInterpMode(*It);
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
						EventTrack->CurveInfo.EventKey.Empty();
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
						VectorTrack->CurveInfo.XKeyInfo.Empty();
						VectorTrack->CurveInfo.YKeyInfo.Empty();
						VectorTrack->CurveInfo.ZKeyInfo.Empty();
						for (int i = 0; i < 3; i++)
						{
							for (auto It(VectorTrack->CurveInfo.CurveObj->FloatCurves[i].GetKeyIterator()); It; ++It)
							{
								if (VectorTrack->CurveInfo.CurveObj->FloatCurves[i].GetNumKeys() > 0)
								{
									FKeyInfo ThisKey;
									ThisKey.Guid = UKismetGuidLibrary::NewGuid().ToString();
									ThisKey.InterpMode = FKeyInfo::ConversionKeyModeToInterpMode(*It);
									ThisKey.BaseKey = *It;
									if (i == 0) VectorTrack->CurveInfo.XKeyInfo.Add(ThisKey);
									else if (i == 1) VectorTrack->CurveInfo.YKeyInfo.Add(ThisKey);
									else if (i == 2) VectorTrack->CurveInfo.ZKeyInfo.Add(ThisKey);
								}
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
						LinearColorTrack->CurveInfo.CurveType = EAdvTimelineType::LinearColor;
						LinearColorTrack->CurveInfo.RKeyInfo.Empty();
						LinearColorTrack->CurveInfo.GKeyInfo.Empty();
						LinearColorTrack->CurveInfo.BKeyInfo.Empty();
						LinearColorTrack->CurveInfo.AKeyInfo.Empty();
						for (int i = 0; i < 4; i++)
						{
							for (auto It(LinearColorTrack->CurveInfo.CurveObj->FloatCurves[i].GetKeyIterator()); It; ++It)
							{
								if (LinearColorTrack->CurveInfo.CurveObj->FloatCurves[i].GetNumKeys() > 0)
								{
									FKeyInfo ThisKey;
									ThisKey.Guid = UKismetGuidLibrary::NewGuid().ToString();
									ThisKey.InterpMode = FKeyInfo::ConversionKeyModeToInterpMode(*It);
									ThisKey.BaseKey = *It;
									if (i == 0) LinearColorTrack->CurveInfo.RKeyInfo.Add(ThisKey);
									else if (i == 1) LinearColorTrack->CurveInfo.GKeyInfo.Add(ThisKey);
									else if (i == 2) LinearColorTrack->CurveInfo.BKeyInfo.Add(ThisKey);
									else if (i == 3) LinearColorTrack->CurveInfo.AKeyInfo.Add(ThisKey);
								}
							}
						}
					}
				}
				break;
				#pragma endregion LinearColor
		}
		return;
	}
	PrintError("ChangeTrackCurve");
}

/** 重置时间线到默认 */
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
		ThisTimeline->PlayStatus = EPlayStatus::Stop;
		ThisTimeline->Position = 0.0f;

		Timelines.Add(ThisTimeline->GuName, *ThisTimeline);
		return;
	}
	PrintError("ResetTimeline");
}

/** 清空时间线轨道 */
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
		return;
	}
	PrintError("ClearTimelineTracks");
}

/** 重置轨道到默认 */
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
					FloatTrack->EventFuncGuName.Empty();
				}
				break;
				#pragma endregion Float

				#pragma region
			case EAdvTimelineType::Event:
				if (FEventTrackInfo* EventTrack = (FEventTrackInfo*)BaseTrack)
				{
					EventTrack->CurveInfo.EventKey.Empty();
					EventTrack->CurveInfo.CurveObj = nullptr;
					EventTrack->EventFuncGuName.Empty();
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
					VectorTrack->EventFuncGuName.Empty();
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
					LinearColorTrack->EventFuncGuName.Empty();
				}
				break;
				#pragma endregion LinearColor
		}
		return;
	}
	PrintError("ResetTrack");
}

/** 删除轨道 */
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
		return;
	}
	PrintError("DelTrack");
}

/** 应用配置到时间线，配合SaveSetting使用 */
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

/** 删除时间线 */
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
	PrintError(LOCTEXT("DelTimelineError", "There is only one timeline left! Or the input parameter is empty!"));
}

/** 清空时间线 */
void UAdvancedTimelineComponent::ClearTimelines()
{
	Timelines.Empty();
}

/** 清空配置，如果有绑定了时间线的配置，不应该删除他 */
void UAdvancedTimelineComponent::ClearSettingList()
{
	TArray<FTimelineSetting> Settings;
	if (Timelines.Num() > 0)
	{
		for (TPair<FString, FTimelineInfo> ThisTimeline : Timelines)
		{
			if (FTimelineSetting* ThisSetting = SettingList.Find(ThisTimeline.Value.SettingGuName))
			{
				Settings.Add(*ThisSetting);
			}
		}
	}
	SettingList.Empty();
	for (FTimelineSetting ThisSetting : Settings)
	{
		SettingList.Add(ThisSetting.GuName, ThisSetting);
	}
}

/** 返回时间线 */
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

/** 返回配置 */
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

/** 查询时间线，C++内部使用 */
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

/** 查询配置，C++内部使用 */
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

/** 查询轨道，C++内部使用 */
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

/** 查询曲线，C++内部使用 */
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

/** 查询关键帧，C++内部使用 */
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
					for (TPair<FString, FFloatTrackInfo> ThisFloatTrack : ThisTimeline.Value.FloatTracks)
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

/** 验证时间线是否可用，C++内部使用 */
bool UAdvancedTimelineComponent::ValidFTimelineInfo(const FTimelineInfo* InTimelineInfo)
{
	if (InTimelineInfo &&
		!InTimelineInfo->GuName.IsEmpty() &&
		!InTimelineInfo->SettingGuName.IsEmpty())
	{
		#pragma region
		if (InTimelineInfo->EventTracks.Num() > 0)
		{
			for (TPair<FString, FEventTrackInfo> ThisTracks : InTimelineInfo->EventTracks)
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

/** 验证时间线配置是否可用，C++内部使用 */
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

/** 验证轨道是否可用，C++内部使用 */
bool UAdvancedTimelineComponent::ValidFTrackInfo(FTrackInfoBase* InTrack)
{
	if (InTrack)
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

/** 验证曲线是否可用，C++内部使用 */
bool UAdvancedTimelineComponent::ValidFCurveInfo(FCurveInfoBase* InCurve)
{
	if (InCurve) {
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

/** 验证关键帧是否可用，C++内部使用 */
bool UAdvancedTimelineComponent::ValidFKeyInfo(FKeyInfo* InKey)
{
	/** 关键帧的值基本都是会默认赋值的，故不需要检查。 */
	if (InKey && !InKey->Guid.IsEmpty()) return true;

	PrintError(LOCTEXT("KeyValidError",
		"Make sure that the GUID of the keyframe and itself have contents!"));
	return false;

}

/** 生成默认的时间线 */
void UAdvancedTimelineComponent::GenerateDefaultTimeline(FTimelineInfo & OutTimeline, FTimelineSetting & OutSetting)
{
	FTimelineInfo DefaultTimeline;
	FTimelineSetting DefaultSetting;

	DefaultSetting.PlayRate = 1.0f;
	DefaultSetting.bIgnoreTimeDilation = false;
	DefaultSetting.bIsLoop = false;
	DefaultSetting.PlayMethod = EPlayMethod::Play;
	DefaultSetting.LengthMode = ELengthMode::Keyframe;
	DefaultSetting.Length = 5.0f;
	DefaultSetting.GuName = UKismetGuidLibrary::NewGuid().ToString();
	DefaultTimeline.GuName = UKismetGuidLibrary::NewGuid().ToString();
	DefaultTimeline.SettingGuName = DefaultSetting.GuName;
	DefaultTimeline.PlayStatus = EPlayStatus::Stop;
	DefaultTimeline.Position = 0.0f;

	OutTimeline = DefaultTimeline;
	OutSetting = DefaultSetting;
}

/** 打印一个默认错误信息 */
void UAdvancedTimelineComponent::PrintError()
{
	const FString ErrorInfo = FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(TEXT("There is an error here, please check him!"), TEXT(LOCTEXT_NAMESPACE), TEXT("GenericError")).ToString();
	UE_LOG(LogAdvTimeline, Error, TEXT("%s"), *ErrorInfo)
}

/** 根据函数名称打印一个错误信息 */
void UAdvancedTimelineComponent::PrintError(const FString InFuncName)
{
	const FString ErrorInfo = LOCTEXT("FuncError", "function has an error!").ToString();
	UE_LOG(LogAdvTimeline, Error, TEXT("%s %s"), *InFuncName, *ErrorInfo)
}

/** 打印一个自定义错误信息 */
void UAdvancedTimelineComponent::PrintError(const FText InErrorInfo)
{
	const FString ErrorInfo = InErrorInfo.ToString();
	UE_LOG(LogAdvTimeline, Error, TEXT("%s"), *ErrorInfo)
}

/** 打印一个报空的错误信息 */
void UAdvancedTimelineComponent::PrintEmptyError()
{
	//GWorld->GetBlueprintObjectsBeingDebugged()
	//UBlueprint* bp=
	//FString StrToken = "AdvancedTimeline";
	const FString ErrorInfo = LOCTEXT("EmptyError", "The incoming parameter cannot be empty!").ToString();
	//CompilerLog->Error(*ErrorInfo, *StrToken);
	UE_LOG(LogAdvTimeline, Error, TEXT("%s"), *ErrorInfo)
}

/** 测试用的，不需要删掉就行 */
UObject* UAdvancedTimelineComponent::TestFunc(UObject* InObject)
{
	return InObject;
}

#undef LOCTEXT_NAMESPACE
