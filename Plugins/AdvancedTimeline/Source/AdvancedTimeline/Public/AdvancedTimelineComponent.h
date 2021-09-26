// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/ActorComponent.h"
#include "Components/TimelineComponent.h"
#include "Engine/TimelineTemplate.h"
#include "DeclarativeSyntaxSupport.h"
#include "AdvancedTimelineComponent.generated.h"

/** �����¼�����Ľṹ�塣
 *	�ٷ��ǰ������¼������������һ��
 *		�μ���FTimeline::AddEvent(...)
 *	�ٷ�������е��¼������Ͷ�Ӧ��ʱ�䱣����һ���ṹ���У��������γ���ͬ����ʱ����в�ͬ�Ķ�Ӧ����(����������ִ�У�)
 *	��������������������Щ�������ĸ��¼�����ġ�
 *	������Ϊ�ٷ���Timeline�ǻ���TimelineTemplate�ģ����Թٷ����¼������Ͷ�Ӧ�����ߵ���Ϣ������FTTEventTrack�ṹ���С�
 *	��FTTEventTrack�ṹ���Ǽ̳���FTTTrackBase�ģ�FTTTrackBase���ж�Ӧ��FName����FTTEventTrack���Ᵽ����UCurveFloat*��
 *	���Թٷ����Զ�Ӧ������Ϊ������TimelineTemplate�С������˾ٽ�Ϊ�ƣ��ʱ����ֱ�ӽ����ߺͶ�Ӧ�ĺ����ϲ�����һ���ɱ�����Լ�����һ���¼������
 *
 *	��Ϊ�����Դ�FTimeline����Private��ԭ�򣬵��±�����Լ���д��һ�顣��Ϊ����Ҫʵ��Timeline�ڵ㣬����������Ӧ���ǲ��ü�����ͼ/Actor�ĳ�ʼ�����������̵ġ�
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
class ADVANCEDTIMELINE_API UAdvancedTimelineComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	/** ʱ�����еĵ�ǰλ�� */
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






	/** ���ȫ��ʱ������Ӧ�ñ�����ʱ���ߺ��ԣ���Ϊ�棬����Ϊ�١�
	*	ʱ�����;���ĳЩ��Ϸ�ṩ�Ķ౶�ٻ򵯳�UIʱ�����绺ʱ��(����Ҳ���Դ���������ͣ��Ϸ��)
	**/
	UPROPERTY()
		uint8 bIgnoreTimeDilation : 1;

	/** �������ĸ���ͬ���͵Ĺ���������Ϣ��һ��ʱ���߿����ж������������ݳ�������Ҳ������ͬ�� */
	UPROPERTY()
		TSet<FAdvEventTrackInfo> AdvEventTracks;
	UPROPERTY()
		TSet<FAdvFloatTrackInfo> AdvFloatTracks;
	UPROPERTY()
		TSet<FAdvVectorTrackInfo> AdvVectorTracks;
	UPROPERTY()
		TSet<FAdvLinearColorTrackInfo> AdvLinearColorTracks;

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


	/** ��ʼ����ʱ���� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control")
		void Play(bool bIsFromStart=false);

	/** ��ʼ���򲥷�ʱ���� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control")
		void Reverse(bool bIsFromEnd=false);

	/** ��ͣ����ʱ���� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control")
		void Pause();

	/** ֹͣ����ʱ���ߣ��Ҳ���λ�ù�0 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control")
		void Reset();

	/** �����Ƿ����ڲ��� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Determining")
		bool IsPlaying() const;

	/** �����Ƿ����ڷ��򲥷� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Determining")
		bool IsReversing() const;

	/** ��ת����λ�á���ΪTickTimeline��д��Ե�ʣ�˳����д��SetPlaybackPosition
	  * @param ��� bFireEvents �� true����ת֮�󴥷�һ��Event
	  * @param ��� bFireEvents �� true����ת֮�󴥷�һ��Update
	 */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Control", meta = (AdvancedDisplay = 1/* ����ǰ1���������Ǹ߼����� */))
		void SetPlaybackPosition(float NewPosition, bool bFireEvents = false, bool bFireUpdate = true, bool bIsPlay = false);

	/** ���ص�ǰ����λ��(s) */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		float GetPlaybackPosition() const;

	/** �����Ƿ�ѭ�� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void SetLooping(bool bNewLooping);

	/** �����Ƿ�ѭ�� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Determining")
		bool IsLooping() const;

	/** ���ò����ٶ� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void SetPlayRate(float NewRate);

	/** ���ص�ǰ�����ٶ� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		float GetPlayRate() const;

	/** ���ص�ǰʱ���߳��� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		float GetTimelineLength() const;

	/** ���õ�ǰʱ���߳��� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void SetTimelineLength(float NewLength);

	/** �����Ƿ������һ���ؼ�֡Ϊ��β�� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void SetTimelineLengthMode(ETimelineLengthMode NewLengthMode);

	/** �����Ƿ����ʱ������ */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void SetIgnoreTimeDilation(bool bNewIgnoreTimeDilation);

	/** �����Ƿ����ʱ������ */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		bool GetIgnoreTimeDilation() const;

	/** �������е�Float��������� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void ChangeFloatTrackCurve(UCurveFloat* NewFloatCurve, FName FloatTrackName);

	/** �������е�Vector��������� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void ChangeVectorTrackCurve(UCurveVector* NewVectorCurve, FName VectorTrackName);

	/** �������е�LinearColor��������� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void ChangeLinearColorTrackCurve(UCurveLinearColor* NewLinearColorCurve, FName LinearColorTrackName);

	/** �������е�Event��������� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Set")
		void ChangeEventTrackCurve(UCurveFloat* NewFloatCurve, const FName&  EventTrackName);

	/** ����ʱ���ߵ�Event����ǩ������ʲô�ã���Ҳ��֪�� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		static UFunction* GetTimelineEventSignature();
	/** ����ʱ���ߵ�Float����ǩ������ʲô�ã���Ҳ��֪�� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		static UFunction* GetTimelineFloatSignature();
	/** ����ʱ���ߵ�Vector����ǩ������ʲô�ã���Ҳ��֪�� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		static UFunction* GetTimelineVectorSignature();
	/** ����ʱ���ߵ�LinearColor����ǩ������ʲô�ã���Ҳ��֪�� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		static UFunction* GetTimelineLinearColorSignature();

	/** ��ȡָ��������ǩ�����͡���ʲô�ã���Ҳ��֪�� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		static ETimelineSigType GetTimelineSignatureForFunction(const UFunction* InFunc);

	/** ���һ��Event�������ǰʱ���� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|AddTrack")
		void AddEventTrack(FAdvEventTrackInfo InEventTrack, bool& bIsSuccess);

	/** ���һ��Vector�������ǰʱ���� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|AddTrack")
		void AddVectorTrack(FAdvVectorTrackInfo InVectorTrack, bool& bIsSuccess);

	/** ���һ��Float�������ǰʱ���� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|AddTrack")
		void AddFloatTrack(FAdvFloatTrackInfo InFloatTrack, bool& bIsSuccess);

	/** ���һ��LinearColor�������ǰʱ���� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|AddTrack")
		void AddLinearColorTrack(FAdvLinearColorTrackInfo InLinearColorTrack, bool& bIsSuccess);

	/** ���õ�ǰʱ���߸�Update����Event */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Event")
		void SetUpdateEvent(FOnTimelineEvent NewTimelinePostUpdateFunc);

	/** ���õ�ǰʱ���߸�Finished������Event */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Event")
		void SetFinishedEvent(const FOnTimelineEvent& NewTimelineFinishedFunc);
	/** ��̬��FinishedEvent���ڲ�������ͼ��ʹ�ã�����֮(һ��Ҳû��Ҫʹ�ð�~������Ҫ��ο�Timeline.cpp�����SetFinishedEvent) */

	/** ����ȫ��������ص����� */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		void GetAllTrackData(TArray<UCurveBase*>& OutCurves, TSet<FName>& OutTrackName, TArray<FName>& OutFuncName) const;

	/** �����κ�һ��ʱ���������һ���ؼ�֡��ʱ��ֵ(���������еģ�������ʱ���ߵ�) */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		float GetLastKeyframeTime() const;

	/** �����κ�һ�����ߵ����һ���ؼ�֡��ʱ��ֵ(�������еģ�����ʱ���ߵ�) */
	UFUNCTION(BlueprintCallable, Category = "AdvancedTimeline|Get")
		float GetCurveLastKeyframeTime(ETrackType InTrackType,UCurveBase* InCurve) const;

	/** ����FTimeline�������ԣ�Ϊ�˸��õĲ���EventTrack������дTickTimeline */
	//TODO: ˼·���ˣ��ȴ���д
	void TickAdvTimeline(float DeltaTime);

	//TODO: ��ռ�������Ĺؼ�֡
	// void ClearTrack();
		
	//TODO: ɾ�������ĳ��ʱ���Ĺؼ�֡��û���ҵ�����false��ɾ���귵��true
	// void DelTrackKey();

	//TODO: ɾ�����
	// void DelTrack();

	//TODO: ����ָ�����ƹ���ϵ����йؼ�֡ʱ���
	// TSet<float> GetAllKeyTimeForTrack();
};
