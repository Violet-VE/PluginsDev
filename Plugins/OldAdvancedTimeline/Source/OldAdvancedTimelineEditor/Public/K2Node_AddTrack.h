// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_AddTrack.generated.h"

/**
 *
 */
UCLASS()
class OLDADVANCEDTIMELINEEDITOR_API UK2Node_AddTrack : public UK2Node
{
	GENERATED_UCLASS_BODY()

	//~ Begin UEdGraphNode Interface.
	/** 这个节点所需要生成的引脚基本都在这边定义生成(引脚，引脚的一些属性(悬浮提示这种)) */
	virtual void AllocateDefaultPins() override;
	/** 设置节点标题 */
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	/** 返回节点悬浮提示文本 */
	virtual FText GetTooltipText() const override;
	/** 节点的主要内容，照抄就行。需要注意要执行的函数所对应的类应该加API宏暴露给其他模块..... */
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	/** 需要重新生成(Reconstruct)节点的时候会通知到。Reconstruct */
	//virtual void PostReconstructNode() override;
	//~ End UEdGraphNode Interface.

	//~ Begin UK2Node Interface
	/** 右键菜单创建节点 */
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	/** 设置右键菜单中的分类 */
	virtual FText GetMenuCategory() const override;
	/** 引脚连接线更改的时候会发出通知 */
	//virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
	//~ End UK2Node Interface

	/** 返回Struct那个引脚 */
	UEdGraphPin* GetStructPin() const { return FindPinChecked(FName("InTrack"));}

	/** 返回Enum那个引脚 */
	UEdGraphPin* GetEnumPin() const { return FindPinChecked(FName("InType")); }
private:

	/** Tooltip text for this node. */
	FText NodeTooltip;

	/** 触发一次刷新，将更新节点的引脚或者其他内容。旨在更新结构体的类型。 */
	//void RefreshNodeOptions();
	
};
