#include "SAdvancedTimelinePinWidget.h"

void SAdvancedTimelinePinWidget::RefreshNameList(const TArray<FString>& InNameList)
{
	//先清空父类的NameList，然后把新的内容塞给他(感觉可能是因为父类是直接拿NameList来处理的。。所以就直接操作他了)
	NameList.Empty();
	for (FString PropName : InNameList)
	{
		TSharedPtr<FName> NamePropertyItem = MakeShareable(new FName(*PropName));
		NameList.Add(NamePropertyItem);
	}
}

void SAdvancedTimelinePinWidget::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj,
	const TArray<FString>& InNameList)
{
	RefreshNameList(InNameList);
	SGraphPinNameList::Construct(SGraphPinNameList::FArguments(), InGraphPinObj, NameList);
}