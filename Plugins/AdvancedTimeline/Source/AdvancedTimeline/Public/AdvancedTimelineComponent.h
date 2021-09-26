// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/ActorComponent.h"
#include "Components/TimelineComponent.h"
#include "Engine/TimelineTemplate.h"
#include "DeclarativeSyntaxSupport.h"
#include "AdvancedTimelineComponent.generated.h"

/** 管理事件轨道的结构体。
 *	官方是把所有事件轨道都塞在了一起：
 *		参见：FTimeline::AddEvent(...)
 *	官方会把所有的事件函数和对应的时间保存在一个结构体中，这样就形成了同样的时间会有不同的对应函数(或许会更方便执行？)
 *	不过这样不方便区分哪些函数是哪个事件轨道的。
 *	不过因为官方的Timeline是基于TimelineTemplate的，所以官方把事件函数和对应的曲线等信息都存在FTTEventTrack结构体中。
 *	而FTTEventTrack结构体是继承自FTTTrackBase的，FTTTrackBase具有对应的FName，而FTTEventTrack额外保存了UCurveFloat*。
 *	所以官方可以对应到，因为存在了TimelineTemplate中。不过此举较为绕，故本插件直接讲曲线和对应的函数合并在了一起，由本插件自己管理一个事件轨道。
 *
 *	因为引擎自带FTimeline各种Private的原因，导致本插件自己重写了一遍。因为不需要实现Timeline节点，所以这个插件应该是不用加入蓝图/Actor的初始化创建等流程的。
 */

 /** Whether or not the timeline should be finished after the specified length, or the last keyframe in the tracks */
UENUM(BlueprintType)
enum class ETrackType : uint8
{
	EventTrack			UMETA(DisplayName = "Event"),
	FloatTrack			UMETA(DisplayName = "Float"),
	VectorTrack			UMETA(DisplayName = "Vector"),
	LinearColorTrack	UMETA(DisplayName = "LinearColor"),
};

USTRUCT(BlueprintType)
struct FAdvTrackInfoBase
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		ETrackType TrackType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FName TrackName;
};

USTRUCT(BlueprintType)
struct FAdvEventTrackInfo : public FAdvTrackInfoBase
{
	GENERATED_USTRUCT_BODY()
public:
	//FAdvEventTrackInfo() 
	//	: Super::TrackType(ETrackType::EventTrack)
	//{}

	/** Curve object used to store keys */
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		UCurveFloat* EventCurve;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FOnTimelineEvent OnEventFunc;
};

USTRUCT(BlueprintType)
struct FAdvFloatTrackInfo : public FAdvTrackInfoBase
{
	GENERATED_USTRUCT_BODY()
public:

		/** Curve object used to store keys */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UCurveFloat* FloatCurve;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FOnTimelineFloat OnEventFunc;
};

USTRUCT(BlueprintType)
struct FAdvVectorTrackInfo : public FAdvTrackInfoBase
{
	GENERATED_USTRUCT_BODY()
public:

		/** Curve object used to store keys */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UCurveVector* VectorCurve;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FOnTimelineVector OnEventFunc;
};

USTRUCT(BlueprintType)
struct FAdvLinearColorTrackInfo : public FAdvTrackInfoBase
{
	GENERATED_USTRUCT_BODY()
public:

		/** Curve object used to store keys */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UCurveLinearColor* LinearColorCurve;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FOnTimelineLinearColor OnEventFunc;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class  UAdvancedTimelineComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	/** 时间线中的当前位置 */
	UPROPERTY()
		float Position;

	/** Specified how the timeline determines its own length (e.g. specified length, last keyframe) */
	UPROPERTY()
		TEnumAsByte<ETimelineLengthMode> LengthMode;

	/** Whether timeline should loop when it reaches the end, or stop */
	UPROPERTY()
		uint8 bLooping : 1;

	/** If playback should move the current position backwards instead of forwards */
	UPROPERTY()
		uint8 bReversePlayback : 1;

	/** Are we currently playing (moving Position) */
	UPROPERTY()
		uint8 bPlaying : 1;

	/** How long the timeline is, will stop or loop at the end */
	UPROPERTY()
		float Length;

	/** How fast we should play through the timeline */
	UPROPERTY()
		float PlayRate;

	/** Called whenever this timeline is playing and updates - done after all delegates are executed and variables updated  */
	UPROPERTY()
		FOnTimelineEvent TimelinePostUpdateFunc;

	/** Called whenever this timeline is finished. Is not called if 'stop' is used to terminate timeline early  */
	UPROPERTY()
		FOnTimelineEvent TimelineFinishedFunc;






	/** 如果全局时间膨胀应该被这条时间线忽略，则为真，否则为假。
	*	时间膨胀就是某些游戏提供的多倍速或弹出UI时的世界缓时。(或许也能以此来设置暂停游戏？)
	**/
	UPROPERTY()
		uint8 bIgnoreTimeDilation : 1;

	/** 保存了四个不同类型的轨道的相关信息。一个时间线可以有多个轨道，其内容除了名称也可以相同。 */
	UPROPERTY()
		TArray<FAdvEventTrackInfo> AdvEventTracks;
	UPROPERTY()
		TArray<FAdvFloatTrackInfo> AdvFloatTracks;
	UPROPERTY()
		TArray<FAdvVectorTrackInfo> AdvVectorTracks;
	UPROPERTY()
		TArray<FAdvLinearColorTrackInfo> AdvLinearColorTracks;

	/** Called whenever this timeline is playing and updates - done after all delegates are executed and variables updated  */
	UPROPERTY()
		FOnTimelineEvent TimelineUpdateFunc;

	/** Called whenever this timeline is finished. Is not called if 'stop' is used to terminate timeline early  */
	UPROPERTY()
		FOnTimelineEvent TimelineFinishFunc;

	//~ Begin ActorComponent Interface.
	virtual void Activate(bool bReset = false) override;
	virtual void Deactivate() override;
	virtual bool IsReadyForOwnerToAutoDestroy() const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~ End ActorComponent Interface.


	/** 开始播放时间线 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control")
		void Play(bool bIsFromStart=false);

	/** 开始反向播放时间线 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control")
		void Reverse(bool bIsFromEnd=false);

	/** 暂停播放时间线 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control")
		void Pause();

	/** 停止播放时间线，且播放位置归0 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control")
		void Reset();

	/** 返回是否正在播放 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Determining")
		bool IsPlaying() const;

	/** 返回是否正在反向播放 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Determining")
		bool IsReversing() const;

	/** 跳转播放位置。因为TickTimeline重写的缘故，顺手重写了SetPlaybackPosition
	  * @param 如果 bFireEvents 是 true，跳转之后触发一次Event
	  * @param 如果 bFireEvents 是 true，跳转之后触发一次Update
	 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control", meta = (AdvancedDisplay = 1/* 除了前1个参数都是高级参数 */))
		void SetPlaybackPosition(float NewPosition, bool bFireEvents = false, bool bFireUpdate = true, bool bIsPlay = false);

	/** 返回当前播放位置(s) */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		float GetPlaybackPosition() const;

	/** 设置是否循环 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void SetLooping(bool bNewLooping);

	/** 返回是否循环 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Determining")
		bool IsLooping() const;

	/** 设置播放速度 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void SetPlayRate(float NewRate);

	/** 返回当前播放速度 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		float GetPlayRate() const;

	/** 返回当前时间线长度 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		float GetTimelineLength() const;

	/** 设置当前时间线长度 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void SetTimelineLength(float NewLength);

	/** 设置是否以最后一个关键帧为结尾。 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void SetTimelineLengthMode(ETimelineLengthMode NewLengthMode);

	/** 设置是否忽略时间膨胀 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void SetIgnoreTimeDilation(bool bNewIgnoreTimeDilation);

	/** 返回是否忽略时间膨胀 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		bool GetIgnoreTimeDilation() const;

	/** 更改已有的Float轨道的曲线 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void ChangeFloatTrackCurve(UCurveFloat* NewFloatCurve, FName FloatTrackName);

	/** 更改已有的Vector轨道的曲线 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void ChangeVectorTrackCurve(UCurveVector* NewVectorCurve, FName VectorTrackName);

	/** 更改已有的LinearColor轨道的曲线 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void ChangeLinearColorTrackCurve(UCurveLinearColor* NewLinearColorCurve, FName LinearColorTrackName);

	/** 更改已有的Event轨道的曲线 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void ChangeEventTrackCurve(UCurveFloat* NewFloatCurve, const FName&  EventTrackName);

	/** 返回时间线的Event函数签名。有什么用？我也不知道 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		static UFunction* GetTimelineEventSignature();
	/** 返回时间线的Float函数签名。有什么用？我也不知道 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		static UFunction* GetTimelineFloatSignature();
	/** 返回时间线的Vector函数签名。有什么用？我也不知道 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		static UFunction* GetTimelineVectorSignature();
	/** 返回时间线的LinearColor函数签名。有什么用？我也不知道 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		static UFunction* GetTimelineLinearColorSignature();

	/** 获取指定函数的签名类型。有什么用？我也不知道 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		static ETimelineSigType GetTimelineSignatureForFunction(const UFunction* InFunc);

	/** 添加一个Event轨道到当前时间线 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|AddTrack")
		void AddEventTrack(FAdvEventTrackInfo InEventTrack, bool& bIsSuccess);

	/** 添加一个Vector轨道到当前时间线 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|AddTrack")
		void AddVectorTrack(FAdvVectorTrackInfo InVectorTrack, bool& bIsSuccess);

	/** 添加一个Float轨道到当前时间线 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|AddTrack")
		void AddFloatTrack(FAdvFloatTrackInfo InFloatTrack, bool& bIsSuccess);

	/** 添加一个LinearColor轨道到当前时间线 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|AddTrack")
		void AddLinearColorTrack(FAdvLinearColorTrackInfo InLinearColorTrack, bool& bIsSuccess);

	/** 设置当前时间线跟Update关联Event */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Event")
		void SetUpdateEvent(FOnTimelineEvent NewTimelinePostUpdateFunc);

	/** 设置当前时间线跟Finished关联的Event */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Event")
		void SetFinishedEvent(const FOnTimelineEvent& NewTimelineFinishedFunc);
	/** 静态的FinishedEvent由于不能在蓝图内使用，故弃之(一般也没必要使用吧~真有需要请参考Timeline.cpp里面的SetFinishedEvent) */

	/** 返回全部曲线相关的数据 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		void GetAllTrackData(TArray<UCurveBase*>& OutCurves, TArray<FName>& OutTrackName, TArray<FName>& OutFuncName) const;

	/** 返回任何一条时间线中最后一个关键帧的时间值(不是曲线中的，是整个时间线的) */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		float GetTrackLastKeyframeTime() const;

	/** 返回任何一条曲线的最后一个关键帧的时间值(是曲线中的，不是时间线的) */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		float GetCurveLastKeyframeTime(ETrackType InTrackType,UCurveBase* InCurve) const;

	/** 由于FTimeline的限制性，为了更好的操作EventTrack，故重写TickTimeline */
	//TODO: 思路错了，等待重写
	void TickAdvTimeline(float DeltaTime);

	//TODO: 清空几个轨道的关键帧
	// void ClearTrack();
		
	//TODO: 删除轨道上某个时间点的关键帧，没有找到返回false，删除完返回true
	// void DelTrackKey();

	//TODO: 删除轨道
	// void DelTrack();

	//TODO: 返回指定名称轨道上的所有关键帧时间点
	// TArray<float> GetAllKeyTimeForTrack();
};
