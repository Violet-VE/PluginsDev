// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Actor.h"
#include "DeclarativeSyntaxSupport.h"
#include "Curves/CurveFloat.h"
#include "Curves/RichCurve.h"
#include "UObject//ObjectMacros.h"
#include "Kismet/KismetGuidLibrary.h"
#include "AdvancedTimelineData.generated.h"

//本地化
#define LOCTEXT_NAMESPACE "AdvTimeline"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAdvancedTimelineEvent, FString, InFuncGuName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAdvancedTimelineEventFloat, FString, InFuncGuName,float, Output);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAdvancedTimelineEventVector, FString, InFuncGuName,FVector,Output);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAdvancedTimelineEventLinearColor, FString, InFuncGuName,FLinearColor,Output);

/** 存放高级时间线的一些相关数据。
 *	单纯的存放数据结构的文件。比如枚举和结构体
 */

/** 存放全局的一些变量(看情况添加，没有就不加了) */
namespace AdvTimelineHelper
{
	/** 变量 */
	
}


/** 枚举 */

/** 高级时间线类型。曲线、关键帧、轨道都用这个来判断 */
UENUM(BlueprintType)
enum class EAdvTimelineType : uint8
{
	Event,
	Float,
	Vector,
	LinearColor
};

/** 时间点模式。即是否使用关键帧作为轨道的播放时间。 */
UENUM(BlueprintType)
enum class ELengthMode : uint8
{
	/** 使用关键帧 */
	Keyframe,
	/** 使用自定义时间 */
	Custom
};

/** 播放状态 */
UENUM(BlueprintType)
enum class EPlayStatus : uint8
{
	/** 正向播放 */
	ForwardPlaying,
	/** 反向播放 */
	ReversePlaying,
	/** 暂停 */
	Pause,
	/** 停止 */
	Stop
};

/** 播放方法 */
UENUM(BlueprintType)
enum class EPlayMethod : uint8
{
	/** 播放 */
	Play,
	/** 始终从头开始播放 */
	PlayOnStart,
	/** 反向播放 */
	Reverse,
	/** 始终从结尾开始播放 */
	ReverseOnEnd
};

/** 结构体 */

/** 曲线的通用属性 */
USTRUCT(BlueprintType)
struct FKeyInfo
{
public:
	GENERATED_USTRUCT_BODY()

	/** GUID */
	UPROPERTY(BlueprintReadOnly)
		FString Guid;

	/** 保存一下关键帧的信息，方便后续查找。 */
	UPROPERTY()
		FRichCurveKey BaseKey;

	/** 当前关键帧的插值模式，只是因为官方的插值模式分了两个 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TEnumAsByte<EInterpCurveMode> InterpMode;
	
	FKeyInfo()
		: InterpMode(EInterpCurveMode::CIM_Unknown)
		, Guid(UKismetGuidLibrary::NewGuid().ToString())
	{ }

	static EInterpCurveMode ConversionKeyModeToInterpMode(const FRichCurveKey InKey)
	{
		if (InKey.InterpMode == RCIM_Constant)
		{
			return CIM_Constant;
		}
		if (InKey.InterpMode == RCIM_Linear)
		{
			return  CIM_Linear;
		}
		if (InKey.InterpMode == RCIM_Cubic)
		{
			if (InKey.TangentMode == RCTM_Break)
			{
				return  CIM_CurveBreak;
			}
			if (InKey.TangentMode == RCTM_Auto)
			{
				return  CIM_CurveAuto;
			}
			if (InKey.TangentMode == RCTM_User)
			{
				return  CIM_CurveUser;
			}
			return  CIM_Linear;
		}
		return  CIM_Linear;
	}
};

/** 曲线的通用属性 */
USTRUCT(BlueprintType)
struct FCurveInfoBase
{
public:
	GENERATED_USTRUCT_BODY()

	FCurveInfoBase()
		: Guid(UKismetGuidLibrary::NewGuid().ToString())
	{ }

	FCurveInfoBase(EAdvTimelineType InType)
		: Guid(UKismetGuidLibrary::NewGuid().ToString())
		, CurveType(InType)
	{ }

	/** GUID */
	UPROPERTY(BlueprintReadOnly)
		FString Guid;
	
	/** 曲线的类型 */
	UPROPERTY(BlueprintReadOnly)
		EAdvTimelineType CurveType;

	/** 曲线没有结束时间和开始时间 */

};
USTRUCT(BlueprintType)
struct FEventKey
{
	GENERATED_USTRUCT_BODY()

	/** GUID */
	UPROPERTY(BlueprintReadOnly)
		FString Guid;

	/** 当前关键帧触发的时间点 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float Time;

	/** 保存一下关键帧的信息，方便后续查找。 */
	UPROPERTY()
		FRichCurveKey BaseKey;

	/** 当前关键帧执行的函数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString EventFuncGuName;

	FEventKey() : Time(0),Guid(UKismetGuidLibrary::NewGuid().ToString()){ }

};

/** 事件曲线 */
USTRUCT(BlueprintType)
struct FEventCurveInfo : public FCurveInfoBase
{
public:
	GENERATED_USTRUCT_BODY()

	FEventCurveInfo(): FCurveInfoBase(EAdvTimelineType::Event){}

	/** 保存曲线的蓝图资源(这里可以决定是否使用蓝图的曲线资源，如果不使用，应该可以实现增删改查关键帧操作，后续补充)
	 *	自己实现一套太麻烦，暂不考虑
	 *	可以通过这个蓝图资源在C++中操作UE官方的曲线，比如添加删除关键帧，获取帧内容等
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UCurveFloat* CurveObj;

	/** 曲线的类型 */
	UPROPERTY()
		TArray<FEventKey> EventKey;
};

/** Float曲线 */
USTRUCT(BlueprintType)
struct FFloatCurveInfo : public FCurveInfoBase
{
public:
	GENERATED_USTRUCT_BODY()

	FFloatCurveInfo() : FCurveInfoBase(EAdvTimelineType::Float) {}

	/** 保存曲线的蓝图资源 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UCurveFloat* CurveObj;

	/** 曲线的类型 */
	UPROPERTY()
		TArray<FKeyInfo> CurveKeyInfo;
};

/** Vector曲线 */
USTRUCT(BlueprintType)
struct FVectorCurveInfo : public FCurveInfoBase
{
public:
	GENERATED_USTRUCT_BODY()

	FVectorCurveInfo() : FCurveInfoBase(EAdvTimelineType::Vector) {}

	/** 保存曲线的蓝图资源 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UCurveVector* CurveObj;

	/** 曲线的类型 */
	UPROPERTY()
		TArray<FKeyInfo> XKeyInfo;

	/** 曲线的类型 */
	UPROPERTY()
		TArray<FKeyInfo> YKeyInfo;

	/** 曲线的类型 */
	UPROPERTY()
		TArray<FKeyInfo> ZKeyInfo;

};

/** LinearColor曲线 */
USTRUCT(BlueprintType)
struct FLinearColorCurveInfo : public FCurveInfoBase
{
public:
	GENERATED_USTRUCT_BODY()

	FLinearColorCurveInfo() : FCurveInfoBase(EAdvTimelineType::LinearColor) {}

	/** 保存曲线的蓝图资源 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UCurveLinearColor* CurveObj;

	/** 曲线的类型 */
	UPROPERTY()
		TArray<FKeyInfo> RKeyInfo;

	/** 曲线的类型 */
	UPROPERTY()
		TArray<FKeyInfo> GKeyInfo;

	/** 曲线的类型 */
	UPROPERTY()
		TArray<FKeyInfo> BKeyInfo;

	/** 曲线的类型 */
	UPROPERTY()
		TArray<FKeyInfo> AKeyInfo;

};

/** 轨道的通用属性 */
USTRUCT(BlueprintType)
struct FTrackInfoBase
{
public:
	GENERATED_USTRUCT_BODY()

	/** GUID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString GuName;

	/** 需要在Update调用的事件函数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString EventFuncGuName;

	FTrackInfoBase()
		: GuName(UKismetGuidLibrary::NewGuid().ToString())
	{ }

	/** 如果GUID都相同了，那就认为是一样的。所以不要给相同的GUID，其他东西不管 */
	friend inline bool operator==(const FTrackInfoBase& A, const FTrackInfoBase& B)
	{
		return
			A.GuName == B.GuName;
	}
};

/** 事件轨道信息 */
USTRUCT(BlueprintType)
struct FEventTrackInfo : public FTrackInfoBase
{
public:
	GENERATED_USTRUCT_BODY()

	/** 轨道对应的曲线信息 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FEventCurveInfo CurveInfo;
};

/** Float轨道信息 */
USTRUCT(BlueprintType)
struct FFloatTrackInfo : public FTrackInfoBase
{
public:
	GENERATED_USTRUCT_BODY()

	/** 轨道对应的曲线信息 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FFloatCurveInfo CurveInfo;
};

/** Vector轨道信息 */
USTRUCT(BlueprintType)
struct FVectorTrackInfo : public FTrackInfoBase
{
public:
	GENERATED_USTRUCT_BODY()

	/** 轨道对应的曲线信息 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVectorCurveInfo CurveInfo;
};

/** LinearColor轨道信息 */
USTRUCT(BlueprintType)
struct FLinearColorTrackInfo : public FTrackInfoBase
{
public:
	GENERATED_USTRUCT_BODY()

	/** 轨道对应的曲线信息 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FLinearColorCurveInfo CurveInfo;
};

/** 时间线设置 */
USTRUCT(BlueprintType)
struct FTimelineSetting
{
public:
	GENERATED_USTRUCT_BODY()

	/** GUID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString GuName;

	/** 是否忽略全局的时间膨胀(没用过，不清楚) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bIgnoreTimeDilation;

	/** Update事件函数，每帧都会执行 */
	UPROPERTY(BlueprintReadOnly)
		FString UpdateEventGuName;

	/** Update事件函数，每帧都会执行 */
	UPROPERTY(BlueprintReadOnly)
		FString FinishedEventGuName;

	/** 播放方法 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		EPlayMethod PlayMethod;

	/** 是否循环 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bIsLoop;

	/** 播放速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float PlayRate;

	/** 时间线长度，一般来说就是轨道长度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float Length;

	/** 是否以关键帧作为时间线的长度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		ELengthMode LengthMode;

	/** 如果GUID都相同了，那就认为是一样的。所以不要给相同的GUID，其他东西不管 */
	friend inline bool operator==(const FTimelineSetting& A, const FTimelineSetting& B)
	{
		return
			A.GuName == B.GuName &&
			A.PlayRate == B.PlayRate &&
			A.bIgnoreTimeDilation == B.bIgnoreTimeDilation &&
			A.bIsLoop == B.bIsLoop &&
			A.UpdateEventGuName == B.UpdateEventGuName &&
			A.FinishedEventGuName == B.FinishedEventGuName;
	}

	/** Hash校验。强调一下，不要给相同的GUID，其他东西不管 */
	friend  inline uint32 GetTypeHash(const FTimelineSetting& Key)
	{
		uint32 Hash = 0;

		Hash = HashCombine(Hash, GetTypeHash(Key.GuName));
		Hash = HashCombine(Hash, GetTypeHash(Key.LengthMode));
		Hash = HashCombine(Hash, GetTypeHash(Key.FinishedEventGuName));
		Hash = HashCombine(Hash, GetTypeHash(Key.UpdateEventGuName));
		Hash = HashCombine(Hash, GetTypeHash(Key.PlayMethod));
		Hash = HashCombine(Hash, GetTypeHash(Key.PlayRate));
		Hash = HashCombine(Hash, GetTypeHash(Key.bIgnoreTimeDilation));
		Hash = HashCombine(Hash, GetTypeHash(Key.bIsLoop));

		return Hash;
	}
};

/** 存放时间线相关的设置 */
USTRUCT(BlueprintType)
struct FTimelineInfo
{
public:
	GENERATED_USTRUCT_BODY()

	/** GUID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString GuName;

	/** 对应的时间线设置，可以切换来快速使用不同的设置(虽然在蓝图里保存再设置貌似也可以) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString SettingGuName;

	/** 保存时间线可能存在的所有Float轨道 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TMap<FString, FFloatTrackInfo> FloatTracks;

	/** 保存时间线可能存在的所有事件轨道 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TMap<FString, FEventTrackInfo> EventTracks;

	/** 保存时间线可能存在的所有Vector轨道 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TMap<FString, FVectorTrackInfo> VectorTracks;

	/** 保存时间线可能存在的所有LinearColor轨道 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TMap<FString, FLinearColorTrackInfo> LinearColorTracks;

	#pragma region
	/** 记录播放状态 */
	UPROPERTY()
		EPlayStatus PlayStatus;

	/** 记录当前位置 */
	UPROPERTY()
		float Position;
	#pragma endregion 保存当前时间线状态的变量

	/** 如果GUID都相同了，那就认为是一样的。所以不要给相同的GUID，其他东西不管 */
	friend inline bool operator==(const FTimelineInfo& A,const FTimelineInfo& B)
	{
		return
			A.GuName == B.GuName &&
			A.SettingGuName == B.SettingGuName;
	}

	/** Hash校验。强调一下，不要给相同的GUID，其他东西不管 */
	friend  inline uint32 GetTypeHash(const FTimelineInfo& Key)
	{
		uint32 Hash = 0;

		Hash = HashCombine(Hash, GetTypeHash(Key.GuName));
		Hash = HashCombine(Hash, GetTypeHash(Key.SettingGuName));
		
		return Hash;
	}
};


UCLASS()
class ADVANCEDTIMELINE_API AAdvancedTimelineData : public AActor
{
	GENERATED_BODY()
};
#undef LOCTEXT_NAMESPACE