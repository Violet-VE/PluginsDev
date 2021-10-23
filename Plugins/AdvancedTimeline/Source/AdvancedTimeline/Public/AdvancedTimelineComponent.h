// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AdvancedTimelineData.h"
#include "AdvancedTimelineComponent.generated.h"

/** 高级时间线。
 *	因为官方的时间线在蓝图中不是很好用，所以自己写了一个......
 **/

/** TODO:
 *	1. 注释
 *	2. Get和Set内容完善
 *	3. 一个时间线对应一个Settings，一个轨道对应一个曲线，一个时间线对应N个轨道，一个曲线对应N个关键帧
 *	4. 有默认值的参数CPP内加注释
 **/

DEFINE_LOG_CATEGORY_STATIC(LogAdvTimeline, Log, All);

//本地化
#define LOCTEXT_NAMESPACE "AdvTimeline"
/** 本地化指南：
 *	只需要使用NSLOCTEXT()或者LOCTEXT()就可以了，剩下的操作需要进编辑器打开本地化Dash面板再改
 */


UCLASS(BlueprintType,meta=(BlueprintSpawnableComponent))
class ADVANCEDTIMELINE_API UAdvancedTimelineComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:

	#pragma region
	/** 一个时间线对应多个轨道，但是同步播放
	 *	一个时间线对应一个配置
	 **/
	UPROPERTY(BlueprintReadOnly)
		TMap<FString, FTimelineInfo> Timelines;
	UPROPERTY(BlueprintReadOnly)
		TMap<FString, FTimelineSetting> SettingList;
	#pragma endregion 变量


	#pragma region
	/** ~ Begin ActorComponent Interface. */
	virtual void Activate(bool bReset = false) override;
	virtual void Deactivate() override;
	virtual bool IsReadyForOwnerToAutoDestroy() const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	/** ~ End ActorComponent Interface. */
	#pragma endregion 覆盖的虚函数


	#pragma region
	/** 返回时间线(一个项目可以有多条时间线) */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		bool GetTimeline(const FString InTimelineGuName, FTimelineInfo& OutTimelineInfo);
	/** 返回时间线设置(一个时间线应该对应一个设置(可以尝试多个设置，类似于软件的多配置选择)) */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		bool GetSetting(const FString InSettingsGuName, FTimelineSetting& OutSetting);
	/** 获取指定轨道当前的播放位置。 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		float GetPlaybackPosition(const FString InTimelineGuName);
	/** 获取是否忽略了全局的时间膨胀 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		bool GetIgnoreTimeDilation(const FString InTimelineGuName);
	/** 获取指定轨道的播放速度 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		float GetPlayRate(const FString InTimelineGuName);
	/** 获取指定轨道的时间模式 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		ELengthMode GetLengthMode(const FString InTimelineGuName);
	/** 获取指定轨道是否开启了循环 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		bool GetIsLoop(const FString InTimelineGuName);
	/** 判断是否只有暂停的轨道 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		bool GetIsPause(const FString InTimelineGuName);
	/** 判断是否只有正在播放的轨道 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		bool GetIsPlaying(const FString InTimelineGuName);
	/** 判断是否全部停止播放了 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		bool GetIsStop(const FString InTimelineGuName);
	/** 获取全部Float轨道的信息 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		void GetAllFloatTracksInfo(const FString InTimelineGuName, TArray<FFloatTrackInfo> & OutFloatTracks);
	/** 获取全部事件轨道的信息 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		void GetAllEventTracksInfo(const FString InTimelineGuName, TArray<FEventTrackInfo> & OutEventTracks);
	/** 获取全部Vector轨道的信息 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		void GetAllVectorTracksInfo(const FString InTimelineGuName, TArray<FVectorTrackInfo> & OutVectorTracks);
	/** 获取全部LinearColor轨道的信息 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		void GetAllLinearColorTracksInfo(const FString InTimelineGuName, TArray<FLinearColorTrackInfo> & OutLinearColorTracks);
	/** 获取时间线中轨道的总数量 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		int GetTrackCount(const FString InTimelineGuName);
	/** 获取时间线中最小关键帧时间 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		float GetMinKeyframeTime(const FString InTimelineGuName);
	/** 获取时间线中最大关键帧时间 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		float GetMaxKeyframeTime(const FString InTimelineGuName);
	/** 获取时间线中最大关键帧时间 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		float GetTimelineLength(const FString InTimelineGuName);
	#pragma endregion Get


	#pragma region
	/** 添加全局的每帧执行的事件函数，反复添加会覆盖，决定好再加 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Add")
		void AddUpdateCallback(const FString InTimelineGuName, const FOnTimelineEvent InUpdateEvent);
	/** 添加结束时执行的函数到轨道，反复添加会覆盖 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Add")
		void AddFinishedCallback(const FString InTimelineGuName, const FOnTimelineEvent InFinishedEvent);
	/** 添加时间线，因为默认有一个(一个项目可以有多条时间线) */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Add")
		void AddTimelineInfo(const FTimelineInfo InTimeline, const FTimelineSetting InSetting);
	/** 添加配置到配置列表，方便管理 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Add")
		void SaveSetting(const FTimelineSetting InTimelineSetting);
	/** 添加一个Float轨道 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Add")
		void AddFloatTrack(const FString InTimelineGuName, FFloatTrackInfo InFloatTrack);
	/** 添加一个事件轨道 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Add")
		void AddEventTrack(const FString InTimelineGuName, FEventTrackInfo InEventTrack);
	/** 添加一个Vector轨道 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Add")
		void AddVectorTrack(const FString InTimelineGuName, FVectorTrackInfo InVectorTrack);
	/** 添加一个LinearColor轨道 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Add")
		void AddLinearColorTrack(const FString InTimelineGuName, FLinearColorTrackInfo InLinearColorTrack);
	/** 添加轨道(通用) */
	UFUNCTION(BlueprintCallable, CustomThunk, meta = (CustomStructureParam = "InTrack"), Category = "AdvancedTimeline|Add")
		void AddTrack(const FString InTimelineGuName, const EAdvTimelineType InTrackType, int32& InTrack);
		void GenericAddTrack(const FString InTimelineGuName, const EAdvTimelineType InTrackType, const UScriptStruct* StructType, void* InTrack);
	DECLARE_FUNCTION(execAddTrack)
	{
		P_GET_PROPERTY(UStrProperty, Z_Param_InTimelineGuid);
		P_GET_ENUM(EAdvTimelineType, Z_Param_InTrackType);
		Stack.Step(Stack.Object, nullptr);
		UStructProperty* StructProperty = ExactCast<UStructProperty>(Stack.MostRecentProperty);
		void* StructPtr = Stack.MostRecentPropertyAddress;
		P_FINISH;

		P_NATIVE_BEGIN;
		P_THIS->GenericAddTrack(Z_Param_InTimelineGuid, EAdvTimelineType(Z_Param_InTrackType), StructProperty->Struct, StructPtr);
		P_NATIVE_END;
	}
	#pragma endregion Add


	#pragma region
	/** 设置轨道的结束时间 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void SetLength(const FString InTimelineGuName, const float InNewTime);
	/** 设置是否忽略了全局的时间膨胀 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void SetIgnoreTimeDilation(const FString InTimelineGuName, const bool IsIgnoreTimeDilation);
	/** 设置指定轨道的播放位置，可以拿来做跳转
	 *	@param bIsFireEvent指示更改位置后是否触发一次事件
	 *	@param bIsFireUpdate指示更改位置后是否触发一次Update
	 *	@param bIsFirePlay指示更改位置后是否继续播放
	 **/
	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = 3), Category = "AdvancedTimeline|Set")
		void SetPlaybackPosition(const FString InTimelineGuName, const float InNewTime, const bool bIsFireEvent, const bool bIsFireUpdate = true, const bool bIsFirePlay = true);
	/** 设置轨道的时间模式 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void SetLengthMode(const FString InTimelineGuName, const ELengthMode InLengthMode);
	/** 设置是否循环轨道，注意时间线和轨道只能设置一个设置了时间线就会覆盖轨道的设置 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void SetLoop(const FString InTimelineGuName, const bool bIsLoop);
	/** 设置整个时间线的播放速度 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void SetPlayRate(const FString InTimelineGuName, const float InNewPlayRate);
	/** 更改轨道的曲线 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void ChangeTrackCurve(const FString InTrackGuName, UCurveBase* InCurve);
	#pragma endregion Set


	#pragma region
	/** 播放时间线 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control")
		void Play(const FString InTimelineGuName, const EPlayMethod PlayMethod = EPlayMethod::Play);
	/** 停止播放时间线，或者暂停 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control")
		void Stop(const FString InTimelineGuName, const bool bIsPause = true);
	/** 重置时间线到默认 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control")
		void ResetTimeline(const FString InTimelineGuName);
	/** 清空时间线 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control")
		void ClearTimelineTracks(const FString InTimelineGuName);
	/** 清空轨道 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control")
		void ResetTrack(const FString InTrackGuName);
	/** 删除轨道 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control")
		void DelTrack(const FString InTrackGuName);
	/** 更改时间线相对于的设置，原有的设置会返回，便于保存 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control")
		void ApplySettingToTimeline(const FString InSettingsGuName, const FString InTimelineGuName, FTimelineSetting& OutOriginSettings);
	/** 删除时间线 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control")
		void DelTimeline(const FString InTimelineGuName);
	/** 清空所有时间线。请注意，时间线始终至少有一个，就算执行了这个函数也会生成一个默认的时间线 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control")
		void ClearTimelines();
	/** 清空配置列表(正在使用中的(即存在Timelines中的)不会清除) */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control")
		void ClearSettingList();
	#pragma endregion Control

	
	#pragma region
	/** 仿官方时间线独立出一个Tick */
	void TickTimeline(const FString InTimelineGuName, float DeltaTime);
	/** 返回时间线(一个项目可以有多条时间线) */
	bool QueryTimeline(const FString InTimelineGuName, FTimelineInfo* & OutTimelineInfo);
	/** 返回时间线设置(一个时间线应该对应一个设置(可以尝试多个设置，类似于软件的多配置选择)) */
	bool QuerySetting(const FString InSettingsGuName, FTimelineSetting* & OutSetting);
	/** 根据Guid查询轨道 */
	bool QueryTrackByGuid(const FString InTrackGuName, EAdvTimelineType& OutTrackType, FTrackInfoBase* & OutTrack, FTimelineInfo* & OutTimeline);
	/** 根据Guid查询曲线 */
	bool QueryCurveByGuid(const FString InCurveGuid, EAdvTimelineType& OutCurveType, FCurveInfoBase* & OutCurve, FTimelineInfo* & OutTimeline);
	/** 根据Guid查询关键帧 */
	bool QueryKeyByGuid(const FString InKeyGuid, EAdvTimelineType& OutKeyType,FRichCurveKey* & OutKey, FTimelineInfo* & OutTimeline);
	/** 验证时间线是否有效 */
	static bool ValidFTimelineInfo(const FTimelineInfo* InTimelineInfo);
	/** 验证时间线设置是否有效 */
	static bool ValidFTimelineSetting(const FTimelineSetting* InSettingsInfo);
	/** 验证轨道是否有效 */
	static bool ValidFTrackInfo(FTrackInfoBase* InTrack);
	/** 验证曲线是否有效 */
	static bool ValidFCurveInfo(FCurveInfoBase* InCurve);
	/** 验证关键帧是否有效 */
	static bool ValidFKeyInfo(FKeyInfo* InKey);
	/** 生成默认的Timeline */
	static void GenerateDefaultTimeline(FTimelineInfo & OutTimeline,FTimelineSetting & OutSetting);
	/** 本地化的输出错误信息(UE_LOG) */
	static void PrintError();
	/** 本地化的输出错误信息(UE_LOG) */
	static void PrintError(const FString InFuncName);
	/** 本地化的输出错误信息(UE_LOG) */
	static void PrintError(const FText InErrorInfo);
	/** 本地化的输出错误信息(UE_LOG) */
	static void PrintEmptyError();
	#pragma endregion 不需要加入GC的函数

};

#undef LOCTEXT_NAMESPACE