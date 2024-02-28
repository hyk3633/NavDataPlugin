
#include "DataGenerator.h"
#include "Components/BoxComponent.h"
#include "Components/BillboardComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include <fstream>
#include "DrawDebugHelpers.h"

using std::vector;
using std::queue;

ADataGenerator::ADataGenerator()
{
	PrimaryActorTick.bCanEverTick = true;

	boxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Area Box"));
	SetRootComponent(boxComponent);
	boxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	boxComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

	billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	billboard->SetupAttachment(RootComponent);

	filePath = FPaths::ProjectDir() + TEXT("/NavData/");
	fileName = TEXT("data.txt");
}

void ADataGenerator::BeginPlay()
{
	Super::BeginPlay();
	InitMapSpecification();
	InitFieldLocations();
	bStartChecking = true;
}

void ADataGenerator::InitMapSpecification()
{
	GetActorBounds(true, origin, extent);

	widthSize = ((extent.X * 2.f) / (float)gridInterval) + 0.5f;
	lengthSize = ((extent.Y * 2.f) / (float)gridInterval) + 0.5f;
}

void ADataGenerator::InitFieldLocations()
{
	grids = vector<vector<GridInfo>>(lengthSize, vector<GridInfo>(widthSize));

	int y, x;
	for (int i = 0; i < lengthSize; i++)
	{
		for (int j = 0; j < widthSize; j++)
		{
			y = (origin.Y + gridInterval * i) - extent.Y;
			x = (origin.X + gridInterval * j) - extent.X;
			grids[i][j].pos = Pos(y, x);
		}
	}
}

void ADataGenerator::DrawDebugGrid()
{
	FVector vector;
	vector.Z = 10;
	for (int i = 0; i < lengthSize; i++)
	{
		for (int j = 0; j < widthSize; j++)
		{
			vector.X = (origin.X + gridInterval * j) - extent.X;
			vector.Y = (origin.Y + gridInterval * i) - extent.Y;
			DrawDebugPoint(GetWorld(), vector, 5.f, FColor::Green, true);
		}
	}
}

void ADataGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bStartChecking)
	{
		CheckTerrain(DeltaTime);
	}
}

void ADataGenerator::CheckTerrain(float deltaTime)
{
	cumulatedTime += deltaTime;
	if (cumulatedTime >= 0.05f)
	{
		if (step == EGeneratingStep::CHECKING_OBSTACLE)
		{
			CheckObstacle();
		}
		else if (step == EGeneratingStep::CHECKING_HEIGHT_DIFFERENCE)
		{
			CheckHeightDifference();
		}
		else if (step == EGeneratingStep::CALCULATING_COST)
		{
			CalculateCost();
		}
		else if (step == EGeneratingStep::SAVING_DATA)
		{
			ExportDataToFile();
		}
		cumulatedTime = 0;
	}
}

void ADataGenerator::CheckObstacle()
{
	int count = 0, i, j;
	FHitResult hit;
	while (count++ < 200 && curIdx < lengthSize * widthSize)
	{
		i = curIdx / widthSize, j = curIdx % widthSize;
		GridInfo& grid = grids[i][j];

		UKismetSystemLibrary::LineTraceSingle
		(
			this,
			FVector(grid.pos.x, grid.pos.y, 1000),
			FVector(grid.pos.x, grid.pos.y, -1000),
			UEngineTypes::ConvertToTraceType(obstacleCheck),
			false,
			TArray<AActor*>(),
			EDrawDebugTrace::None,
			hit,
			true
		);
		grid.isPassable = !hit.bBlockingHit;

		// 현재 그리드 위치에 장애물이 있을 경우 장애물 가중치 추가
		if (hit.bBlockingHit)
		{
			grid.extraCost = obstacleCost * 100;
			blockedGrids.push({ i,j });

			DrawDebugPoint(GetWorld(), FVector(grid.pos.x, grid.pos.y, hit.ImpactPoint.Z + 10), 7.5f, FColor::Red, true);
		}
		// 현재 그리드 위치에 장애물이 없을 경우 높이 값 체크
		else
		{
			UKismetSystemLibrary::LineTraceSingle
			(
				this,
				FVector(grid.pos.x, grid.pos.y, 1000),
				FVector(grid.pos.x, grid.pos.y, -1000),
				UEngineTypes::ConvertToTraceType(heightCheck),
				false,
				TArray<AActor*>(),
				EDrawDebugTrace::None,
				hit,
				true
			);
			grid.height = hit.ImpactPoint.Z;
			grid.isPassable = hit.bBlockingHit; // 트레이스 충돌 결과가 없으면 바닥이 없는 곳

			if(hit.bBlockingHit)
				DrawDebugPoint(GetWorld(), FVector(grid.pos.x, grid.pos.y, hit.ImpactPoint.Z + 10), 7.5f, FColor::Green, true);
		}
		curIdx++;
	}

	if (curIdx >= lengthSize * widthSize)
	{
		step = EGeneratingStep::CHECKING_HEIGHT_DIFFERENCE;
		curIdx = 0;
		cumulatedTime = 0.1f;
	}
}

void ADataGenerator::CheckHeightDifference()
{
	int count = 0, i, j;
	while (count++ < 200 && curIdx < lengthSize * widthSize)
	{
		i = curIdx / widthSize, j = curIdx % widthSize;
		if (grids[i][j].isPassable)
		{
			GridInfo& grid = grids[i][j];
			for (int idx = 0; idx < 4; idx++)
			{
				const int nextI = i + front[idx].y, nextJ = j + front[idx].x;
				if (nextI < 0 || nextI >= lengthSize || nextJ < 0 || nextJ >= widthSize)
					continue;
				if (grids[nextI][nextJ].isPassable == false)
					continue;
				if (grids[nextI][nextJ].height - grid.height <= heightDifferenceLimit)
					continue;

				blockedGrids.push({ i,j });
				grid.extraCost = (obstacleCost - 2) * 100;
				DrawDebugPoint(GetWorld(), FVector(grid.pos.x, grid.pos.y, grid.height + 10), 7.5f, FColor::Purple, true);
				break;
			}
		}
		curIdx++;
	}
	FString msg = FString::Printf(TEXT("Height difference checking has been %d percent progressed."), (int)(curIdx / (float)(lengthSize * widthSize) * 100));
	GEngine->AddOnScreenDebugMessage(1, 1.f, FColor::Black, msg);

	if (curIdx >= lengthSize * widthSize)
	{
		step = EGeneratingStep::CALCULATING_COST;
		curIdx = 0;
		cumulatedTime = 0.1f;
	}
}

void ADataGenerator::CalculateCost()
{
	int count = 0;
	while (count++ < 100 && !blockedGrids.empty())
	{
		BFS();
	}

	if (blockedGrids.empty())
	{
		step = EGeneratingStep::SAVING_DATA;
		cumulatedTime = 0.1f;
	}
}

void ADataGenerator::BFS()
{
	queue<std::pair<int, int>> nextGrid;
	std::pair<int,int> p = blockedGrids.front();
	nextGrid.push(p);
	blockedGrids.pop();
	int currentCost = (grids[p.first][p.second].extraCost ? (grids[p.first][p.second].extraCost / 100) : obstacleCost) - 2;
	while (!nextGrid.empty() && currentCost >= 4)
	{
		queue<std::pair<int, int>> tempQ;
		while (!nextGrid.empty())
		{
			int ci = nextGrid.front().first, cj = nextGrid.front().second;
			nextGrid.pop();

			for (int idx = 0; idx < 8; idx++)
			{
				int ni = ci + front[idx].y, nj = cj + front[idx].x;
				if (ni < 0 || ni >= lengthSize || nj < 0 || nj >= widthSize)
					continue;
				if (grids[ni][nj].extraCost / 100 >= currentCost)
					continue;

				grids[ni][nj].extraCost = currentCost * 100;

				if (currentCost > 4)
					tempQ.push({ ni,nj });

				if (currentCost < 8)
				{
					DrawDebugPoint(GetWorld(), FVector(grids[ni][nj].pos.x, grids[ni][nj].pos.y, grids[ni][nj].height + 10.f), 7.5f, FColor::Yellow, true);
				}
				else
				{
					DrawDebugPoint(GetWorld(), FVector(grids[ni][nj].pos.x, grids[ni][nj].pos.y, grids[ni][nj].height + 10.f), 7.5f, FColor::Orange, true);
				}
			}
		}
		nextGrid = tempQ;
		currentCost -= 2;
	}
}

void ADataGenerator::ExportDataToFile()
{
	FString content;

	content += FString::Printf(TEXT("%f"), origin.X) + " " + FString::Printf(TEXT("%f"), origin.Y) + " " + FString::Printf(TEXT("%f"), origin.Z) + "\n";
	content += FString::Printf(TEXT("%f"), extent.X) + " " + FString::Printf(TEXT("%f"), extent.Y) + " " + FString::Printf(TEXT("%f"), extent.Z) + "\n";
	content += FString::FromInt(gridInterval) + "\n";
	content += FString::FromInt(lengthSize) + "\n";
	content += FString::FromInt(widthSize) + "\n";

	for (int i = 0; i < lengthSize; i++)
	{
		for (int j = 0; j < widthSize; j++)
		{
			content += FString::FromInt(grids[i][j].pos.x) + " " 
					 + FString::FromInt(grids[i][j].pos.y) + " " 
					 + FString::Printf(TEXT("%f"), grids[0][0].height) + " " 
					 + FString::FromInt(grids[i][j].isPassable) + " " 
					 + FString::FromInt(grids[i][j].extraCost) + "\n";
		}
	}
	FFileHelper::SaveStringToFile(content, *(filePath + fileName), FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);

	bStartChecking = false;

	GEngine->AddOnScreenDebugMessage(0, 5, FColor::Blue, FString("Complete!"));
}

