// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <queue>
#include "DataGenerator.generated.h"

struct Pos
{
	int y, x;

	Pos(int _y, int _x) : y(_y), x(_x) {}
	Pos() : y(0), x(0) {}
	Pos operator+(Pos& _Pos)
	{
		Pos ret{ y + _Pos.y , x + _Pos.x };
		return ret;
	}
	bool operator<(const Pos& other) const
	{
		if (y != other.y)
			return y < other.y;
		return x < other.x;
	}
	bool operator!=(const Pos& other) const
	{
		return (y != other.y || x != other.x);
	}
	friend Pos operator+(const Pos& a, Pos& b)
	{
		Pos ret{ a.y + b.y, a.x + b.x };
		return ret;
	}
	friend bool operator==(const Pos& L, const Pos& R) {
		return (L.y == R.y && L.x == R.x);
	}
	float GetDistance(Pos& other)
	{
		return sqrt(pow(x - other.x, 2)) + sqrt(pow(y - other.y, 2));
	}
};

struct GridInfo
{
	Pos pos;
	float height;
	bool isPassable;
	int extraCost;
};

enum class EGeneratingStep
{
	CHECKING_OBSTACLE,
	CHECKING_HEIGHT_DIFFERENCE,
	CALCULATING_COST,
	SAVING_DATA
};

class UBoxComponent;
class UBillboardComponent;

UCLASS()
class NAVDATAGENERATORPLUGIN_API ADataGenerator : public AActor
{
	GENERATED_BODY()
	
public:	

	ADataGenerator();

protected:

	virtual void BeginPlay() override;
	
	void DrawDebugGrid();

	void InitMapSpecification();

	void InitFieldLocations();

public:	

	virtual void Tick(float DeltaTime) override;

protected:

	void CheckTerrain(float deltaTime);

	void CheckObstacle();

	void CheckHeightDifference();

	void CalculateCost();

	void BFS();

	void ExportDataToFile();

private:

	UPROPERTY(EditInstanceOnly)
	UBillboardComponent* billboard;

	UPROPERTY(EditInstanceOnly)
	UBoxComponent* boxComponent;

	UPROPERTY(EditInstanceOnly, Category = "Nav Option", meta = (AllowPrivateAccess = "true", ClampMin = "1", ClampMax = "100"))
	int gridInterval;

	UPROPERTY(EditInstanceOnly, Category = "Nav Option", meta = (AllowPrivateAccess = "true"))
	TEnumAsByte<ECollisionChannel> obstacleCheck = ECC_Visibility;

	UPROPERTY(EditInstanceOnly, Category = "Nav Option", meta = (AllowPrivateAccess = "true"))
	TEnumAsByte<ECollisionChannel> heightCheck = ECC_Visibility;

	UPROPERTY(EditInstanceOnly, Category = "Nav Option", meta = (AllowPrivateAccess = "true"))
	FString fileName;

	FString filePath;

	FVector origin;
	FVector extent;

	int lengthSize;
	int widthSize;

	std::vector<std::vector<GridInfo>> grids;
	std::queue<std::pair<int,int>> blockedGrids;

	bool bStartChecking;
	EGeneratingStep step = EGeneratingStep::CHECKING_OBSTACLE;
	float cumulatedTime;
	int curIdx;

	int obstacleCost = 12;
	int heightDifferenceLimit = 20;

	Pos front[8] =
	{
		Pos { -1, 0},
		Pos { 0, -1},
		Pos { 1, 0},
		Pos { 0, 1},
		Pos {-1, -1},
		Pos {1, -1},
		Pos {1, 1},
		Pos {-1, 1}
	};
};
