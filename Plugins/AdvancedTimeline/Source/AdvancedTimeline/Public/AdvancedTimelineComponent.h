// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AdvancedTimelineData.h"
#include "AdvancedTimelineComponent.generated.h"

/** 高级时间线。
 *	因为官方的时间线在蓝图中不是很好用，所以自己写了一个......
 **/

/** 定义日志分类 */
DEFINE_LOG_CATEGORY_STATIC(LogAdvTimeline, Log, All);

/** 本地化 */
#define LOCTEXT_NAMESPACE "AdvTimeline"
/** 本地化指南：
 *	只需要使用NSLOCTEXT()或者LOCTEXT()就可以了，剩下的操作需要进编辑器打开本地化Dash面板再改
 */

UCLASS(BlueprintType,meta=(BlueprintSpawnableComponent))
class ADVANCEDTIMELINE_API UAdvancedTimelineComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	#pragma region
	/** 一个时间线对应多个轨道，但是同步播放
	 *	一个时间线对应一个配置
	 *	播放时执行函数使用动态多播委托，因此播放前记得绑定一下
	 **/
	/** 保存了所有的时间线，不建议直接使用 */
	UPROPERTY(BlueprintReadOnly)
		TMap<FString, FTimelineInfo> Timelines;
	/** 保存了所有的配置列表，不建议直接使用 */
	UPROPERTY(BlueprintReadOnly)
		TMap<FString, FTimelineSetting> SettingList;

	/** 绑定事件委托。使用时Switch字符串来判断哪个是需要执行的函数。记得命名唯一 */
	UPROPERTY(BlueprintAssignable)
		FOnAdvancedTimelineEvent AdvTimelineEventDelegate;
	/** 绑定Update委托。使用时Switch字符串来判断哪个是需要执行的函数。记得命名唯一 */
	UPROPERTY(BlueprintAssignable)
		FOnAdvancedTimelineEvent AdvTimelineUpdateDelegate;
	/** 绑定Finished委托。使用时Switch字符串来判断哪个是需要执行的函数。记得命名唯一 */
	UPROPERTY(BlueprintAssignable)
		FOnAdvancedTimelineEvent AdvTimelineFinishedDelegate;
	/** 绑定Float委托。使用时Switch字符串来判断哪个是需要执行的函数。记得命名唯一 */
	UPROPERTY(BlueprintAssignable)
		FOnAdvancedTimelineEventFloat AdvTimelineFloatDelegate;
	/** 绑定Vector委托。使用时Switch字符串来判断哪个是需要执行的函数。记得命名唯一 */
	UPROPERTY(BlueprintAssignable)
		FOnAdvancedTimelineEventVector AdvTimelineVectorDelegate;
	/** 绑定LinearColor委托。使用时Switch字符串来判断哪个是需要执行的函数。记得命名唯一 */
	UPROPERTY(BlueprintAssignable)
		FOnAdvancedTimelineEventLinearColor AdvTimelineLinearColorDelegate;
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
	/** 获取某一条时间线(一个项目可以有多条时间线) */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		bool GetTimeline(const FString InTimelineGuName, FTimelineInfo& OutTimelineInfo);
	/** 获取时间线设置(一个时间线应该对应一个设置(可以切换多个设置，类似于软件的多配置选择)) */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		bool GetSetting(const FString InSettingsGuName, FTimelineSetting& OutSetting);
	/** 获取指定时间线当前的播放位置。 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		float GetPlaybackPosition(const FString InTimelineGuName);
	/** 获取是否忽略了全局的时间膨胀 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		bool GetIgnoreTimeDilation(const FString InTimelineGuName);
	/** 获取指定时间线的播放速度 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		float GetPlayRate(const FString InTimelineGuName);
	/** 获取指定时间线的长度模式 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		ELengthMode GetLengthMode(const FString InTimelineGuName);
	/** 获取指定时间线是否开启了循环 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		bool GetIsLoop(const FString InTimelineGuName);
	/** 判断时间线是否暂停了 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		bool GetIsPause(const FString InTimelineGuName);
	/** 判断时间线是否正在播放 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		bool GetIsPlaying(const FString InTimelineGuName);
	/** 判断时间线是否停止播放 */
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
	/** 获取时间线长度。Update时间取决于这个 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		float GetTimelineLength(const FString InTimelineGuName);
	/** 获取时间线播放状态 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		EPlayStatus GetPlayStatus(const FString InTimelineGuName);
	#pragma endregion Get


	#pragma region
	/** 添加全局的每帧执行的事件函数，反复添加会覆盖，决定好再加 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Add")
		void AddUpdateCallback(const FString InTimelineGuName, const FString InUpdateEventGuName);
	/** 添加结束时执行的函数到轨道，反复添加会覆盖 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Add")
		void AddFinishedCallback(const FString InTimelineGuName, const FString InFinishedEventGuName);
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
	/** 生成默认的Timeline */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Add")
	static void GenerateDefaultTimeline(FTimelineInfo & OutTimeline, FTimelineSetting & OutSetting);
	/** 添加轨道(通用) */
	UFUNCTION(BlueprintCallable, CustomThunk, meta = (CustomStructureParam = "InTrack"), Category = "AdvancedTimeline|Add")
		void AddTrack(const FString InTimelineGuName, const EAdvTimelineType InTrackType, int32 InTrack);// 这里的int32只是占位，实际不会用。需要注意的是如果单写了&还是会放在输出那边
		void GenericAddTrack(const FString InTimelineGuName, const EAdvTimelineType InTrackType, const UScriptStruct* StructType, void* InTrack);// 上面那个函数实际执行的函数
	DECLARE_FUNCTION(execAddTrack)// 自定义一个Thunk函数，一定要exec(小写)+需要自定义的函数名称。(例如AddTrack需要自定义，这个名称就写execAddTrack，不可取其他名称)
	{
		// 获取属性，这里需要啥类型可以写一个带那个类型的UFUNCTION，然后去对应的Intermediate\Build\Win64\UE4Editor\Inc路径下找对应的generated.h文件里面的对应函数直接Copy即可
		P_GET_PROPERTY(UStrProperty, Z_Param_InTimelineGuid);
		P_GET_ENUM(EAdvTimelineType, Z_Param_InTrackType);
		// 下面三行只是为了获取到传入的结构体，如果不需要结构体不写也行
		Stack.Step(Stack.Object, nullptr);
		UStructProperty* StructProperty = ExactCast<UStructProperty>(Stack.MostRecentProperty);
		void* StructPtr = Stack.MostRecentPropertyAddress;
		P_FINISH;// 标识获取属性完成了

		P_NATIVE_BEGIN;// 表示开始调用函数
		// 用P_THIS来获取this指针
		P_THIS->GenericAddTrack(Z_Param_InTimelineGuid, EAdvTimelineType(Z_Param_InTrackType), StructProperty->Struct, StructPtr);
		P_NATIVE_END;// 表示调用结束

		// 如果需要更自定义的函数实现，例如动态增删引脚，可以使用K2Node。具体可以参考房燕良老师的文章
	}
	#pragma endregion Add


	#pragma region
	/** 设置轨道的长度 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void SetLength(const FString InTimelineGuName, const float InNewTime);
	/** 设置是否忽略了全局的时间膨胀 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void SetIgnoreTimeDilation(const FString InTimelineGuName, const bool IsIgnoreTimeDilation);
	/** 设置指定轨道的播放位置，可以拿来做跳转
	 *	@param bIsFireEvent指示更改位置后是否触发一次事件
	 *	@param bIsFireUpdate指示更改位置后是否触发一次Update
	 **/
	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = 3), Category = "AdvancedTimeline|Set")
		void SetPlaybackPosition(const FString InTimelineGuName, const float InNewTime, const bool bIsFireEvent, const bool bIsFireUpdate = true);
	/** 设置时间线的长度模式 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void SetLengthMode(const FString InTimelineGuName, const ELengthMode InLengthMode);
	/** 设置是否循环时间线 */
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
	/** 重置轨道 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control")
		void ResetTrack(const FString InTrackGuName);
	/** 删除轨道 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control")
		void DelTrack(const FString InTrackGuName);
	/** 更改时间线相对应的设置，原有的设置会返回，便于保存 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control")
		void ApplySettingToTimeline(const FString InSettingsGuName, const FString InTimelineGuName, FTimelineSetting& OutOriginSettings);
	/** 删除时间线 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control")
		void DelTimeline(const FString InTimelineGuName);
	/** 清空所有时间线。 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control")
		void ClearTimelines();
	/** 清空配置列表(存在Timelines中的不会清除) */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control")
		void ClearSettingList();
	#pragma endregion Control

	
	#pragma region
	/** 仿官方时间线独立出一个Tick */
	void TickTimeline(const FString InTimelineGuName, float DeltaTime);
	/** 根据GuName查询时间线 */
	bool QueryTimeline(const FString InTimelineGuName, FTimelineInfo* & OutTimelineInfo);
	/** 根据GuName查询时间线配置 */
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
	/** 本地化的输出错误信息(UE_LOG)，如果不需要具体的错误信息可以直接使用这个 */
	static void PrintError();
	/** 本地化的输出错误信息(UE_LOG)，根据给的函数名称输出：某个函数有错误 */
	static void PrintError(const FString InFuncName);
	/** 本地化的输出错误信息(UE_LOG) */
	static void PrintError(const FText InErrorInfo);
	/** 本地化的输出错误信息(UE_LOG) */
	static void PrintEmptyError();
	#pragma endregion 不需要加入GC的函数

	/** 用来做测试的函数，没需要的话直接删了就可以 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Test")
		UObject* TestFunc(UObject* InObject);
};

#undef LOCTEXT_NAMESPACE