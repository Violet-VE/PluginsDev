// Fill out your copyright notice in the Description page of Project Settings.

#include "FreeTimeline.h"


// Sets default values for this component's properties
UFreeTimeline::UFreeTimeline()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UFreeTimeline::BeginPlay()
{
	Super::BeginPlay();

	// ...
}


// Called every frame
void UFreeTimeline::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

