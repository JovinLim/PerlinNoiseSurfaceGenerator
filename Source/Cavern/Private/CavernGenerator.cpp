// Fill out your copyright notice in the Description page of Project Settings.


#include "CavernGenerator.h"
#include "MeshDescription.h"
#include "MeshDescriptionBuilder.h"
#include "StaticMeshAttributes.h"
#include "Utils/VoxelMeshData.h"
#include "ProceduralMeshComponent.h"

// Sets default values
ACavernGenerator::ACavernGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>("Mesh");

	// Mesh Settings
	Mesh->SetCastShadow(false);

	// Set Mesh as root
	SetRootComponent(Mesh);

}

// Called when the game starts or when spawned
void ACavernGenerator::BeginPlay()
{
	Super::BeginPlay();
	//TArray<TArray<TArray<float>>> matrix = GenerateMatrix();
	//PrintMatrix(matrix[0]);
	
	//PERLIN STUFF
	//TArray<TArray<float>> matrixPerlin = PerlinNoise(perlinSeed, x_size, y_size);
	//PrintMatrix(matrixPerlin);

	//DEBUGGING POINTS
	//ShowDebugGeometry(matrix);
	 

	//MAKE MESH
	//Setup();
	//Generate3DHeightMap(matrix);
	//GenerateMesh(matrix);
	//UE_LOG(LogTemp, Warning, TEXT("Vertex Count : %d"), VertexCount);
	//ApplyMesh();

	//CUSTOM MESH
	CustomMesh();
	ApplyMesh();

	UE_LOG(LogTemp, Warning, TEXT("Successfully set static mesh"));
}

// Called every frame
void ACavernGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACavernGenerator::Setup()
{
	// Initialize Voxels
	Voxels.SetNum((z_size + 1) * (y_size + 1) * (x_size + 1));
}

void ACavernGenerator::Generate3DHeightMap(TArray<TArray<TArray<float>>> matrix)
{
	for (int x = 0; x < x_size; ++x)
	{
		for (int y = 0; y < y_size; ++y)
		{
			for (int z = 0; z < z_size; ++z)
			{
				Voxels[GetVoxelIndex(x, y, z)] = matrix[z][y][x];	
			}
		}
	}
}

void ACavernGenerator::March(int x, int y, int z, const float Cube[8])
{
	int VertexMask = 0;
	FVector EdgeVertex[12];

	for (int i = 0; i < 8; i++) {
		if (Cube[i] <= SurfaceLevel) VertexMask |= 1 << i;
	}

	const int EdgeMask = CubeEdgeFlags[VertexMask];
	FString print = FString::FromInt(EdgeMask);
	//UE_LOG(LogTemp, Warning, TEXT("Isoval : %s"), *print);

	if (EdgeMask == 0) return;

	// Find intersection points
	for (int i = 0; i < 12; ++i)
	{
		if ((EdgeMask & 1 << i) != 0)
		{
			const float Offset = Interpolation ? GetInterpolationOffset(Cube[EdgeConnection[i][0]], Cube[EdgeConnection[i][1]]) : 0.5f;

			EdgeVertex[i].X = x + (VertexOffset[EdgeConnection[i][0]][0] + Offset * EdgeDirection[i][0]);
			EdgeVertex[i].Y = y + (VertexOffset[EdgeConnection[i][0]][1] + Offset * EdgeDirection[i][1]);
			EdgeVertex[i].Z = z + (VertexOffset[EdgeConnection[i][0]][2] + Offset * EdgeDirection[i][2]);
		}
	}

	// Save triangles, at most can be 5
	for (int i = 0; i < 5; ++i)
	{
		if (TriangleConnectionTable[VertexMask][3 * i] < 0) break;

		auto V1 = EdgeVertex[TriangleConnectionTable[VertexMask][3 * i]] * gridSize;
		auto V2 = EdgeVertex[TriangleConnectionTable[VertexMask][3 * i + 1]] * gridSize;
		auto V3 = EdgeVertex[TriangleConnectionTable[VertexMask][3 * i + 2]] * gridSize;

		auto Normal = FVector::CrossProduct(V2 - V1, V3 - V1);
		auto Color = FColor::MakeRandomColor();

		Normal.Normalize();

		MeshData.Vertices.Append({ V1, V2, V3 });

		MeshData.Triangles.Append({
			VertexCount + TriangleOrder[0],
			VertexCount + TriangleOrder[1],
			VertexCount + TriangleOrder[2]
			});

		MeshData.Normals.Append({
			Normal,
			Normal,
			Normal
			});

		MeshData.Colors.Append({
			Color,
			Color,
			Color
			});

		VertexCount += 3;
	}
	
}

int ACavernGenerator::GetVoxelIndex(int X, int Y, int Z) const
{
	return Z * (z_size + 1) * (y_size + 1) + Y * (x_size + 1) + X;
	//return Z * (z_size) * (y_size) + Y * (x_size) + X;
}

float ACavernGenerator::GetInterpolationOffset(float V1, float V2) const
{
	const float Delta = V2 - V1;
	return Delta == 0.0f ? SurfaceLevel : (SurfaceLevel - V1) / Delta;
}

int ACavernGenerator::getCubeIndex(float Cube[8])
{
	int cubeindex = 0;
	if (Cube[0] < SurfaceLevel) cubeindex |= 1;
	if (Cube[1] < SurfaceLevel) cubeindex |= 2;
	if (Cube[2] < SurfaceLevel) cubeindex |= 4;
	if (Cube[3] < SurfaceLevel) cubeindex |= 8;
	if (Cube[4] < SurfaceLevel) cubeindex |= 16;
	if (Cube[5] < SurfaceLevel) cubeindex |= 32;
	if (Cube[6] < SurfaceLevel) cubeindex |= 64;
	if (Cube[7] < SurfaceLevel) cubeindex |= 128;
	FString print = FString::FromInt(cubeindex);
	UE_LOG(LogTemp, Warning, TEXT("CubeIndex : %s"), *print);
	return cubeindex;
}

void ACavernGenerator::PostEditChangeProperty(FPropertyChangedEvent& e)
{
	Super::PostEditChangeProperty(e);
	FName PropertyName = (e.Property != NULL) ? e.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ACavernGenerator, perlinFrequency) || 
		PropertyName == GET_MEMBER_NAME_CHECKED(ACavernGenerator, perlinOctaves)   || 
		PropertyName == GET_MEMBER_NAME_CHECKED(ACavernGenerator, perlinSeed)      ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ACavernGenerator, x_size)          ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ACavernGenerator, y_size)          ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ACavernGenerator, z_size)          ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ACavernGenerator, gridSize)        ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ACavernGenerator, NoiseOffset)       
		) {

		MeshData.Clear();
		//Mesh->ClearAllMeshSections();
		CustomMesh();
		//ApplyMesh();
		Mesh->UpdateMeshSection_LinearColor(
			0,
			MeshData.Vertices,
			MeshData.Normals,
			MeshData.UV0,
			MeshData.Colors,
			TArray<FProcMeshTangent>(),
			true
			);

		///* Because you are inside the class, you should see the value already changed */
		//if (MyBool) doThings(); // This is how you access MyBool.
		//else undoThings();

		///* if you want to use bool property */
		//UBoolProperty* prop = static_cast<UBoolProperty*>(e.Property);
		//if (prop->GetPropertyValue())
		//	dothings()
		//else
		//	undothings()
	}
}

void ACavernGenerator::GenerateMesh(TArray<TArray<TArray<float>>> matrix)
{
	// Triangulation order
	if (SurfaceLevel > 0.0f)
	{
		TriangleOrder[0] = 0;
		TriangleOrder[1] = 1;
		TriangleOrder[2] = 2;
	}
	else
	{
		TriangleOrder[0] = 2;
		TriangleOrder[1] = 1;
		TriangleOrder[2] = 0;
	}

	float Cube[8];

	for (int z = 0; z < z_size-1; z++) {
		for (int y = 0; y < y_size-1; y++) {
			for (int x = 0; x < x_size-1; x++) {
				for (int i = 0; i < 8; ++i)
				{
					//Cube[i] = Voxels[GetVoxelIndex(x + VertexOffset[i][0], y + VertexOffset[i][1], z + VertexOffset[i][2])];
					//Cube[i] = Voxels[GetVoxelIndex(x, y, z)];
					Cube[i] = matrix[z + VertexOffset[i][2]][y + VertexOffset[i][1]][x + VertexOffset[i][0]];
				}

				//getCubeIndex(Cube);
				March(x, y, z, Cube);
			}
		}
	}
}

void ACavernGenerator::CustomMesh()
{
	TArray<TArray<float>> matrixPerlin = PerlinNoise(perlinSeed, y_size, z_size);

	//FVector V1 = FVector( 0.0, 0.0, 0.0);
	//FVector V2 = FVector( 0.0f, 100, 0);
	//FVector V3 = FVector( 0.0f, 0, 100);
	//FVector V4 = FVector( 0.0f, 100, 100);
	//MeshData.Vertices.Append({ V1, V3, V4 });
	//MeshData.Triangles.Append({
	//			VertexCount,
	//			VertexCount + 1,
	//			VertexCount + 2,
	//	});
	//auto Normal = FVector::CrossProduct(V3 - V1, V4 - V1);
	//MeshData.Normals.Append({
	//	Normal,
	//	Normal,
	//	Normal,
	//	});
	//auto Color = FColor::MakeRandomColor();
	//MeshData.Colors.Append({
	//Color,
	//Color,
	//Color
	//	});
	//VertexCount += 3;
	//MeshData.Vertices.Append({ V4, V2, V1 });
	//MeshData.Triangles.Append({
	//			VertexCount,
	//			VertexCount + 1,
	//			VertexCount + 2,
	//	});
	//auto Normal2 = FVector::CrossProduct(V2 - V4, V1 - V4);
	//MeshData.Normals.Append({
	//	Normal2,
	//	Normal2,
	//	Normal2,
	//	});
	//MeshData.Colors.Append({
	//Color,
	//Color,
	//Color
	//	});
	//VertexCount += 3;



	for (int z = 1; z < z_size - 1; z++) {
		for (int y = 1; y < y_size - 1; y++) {
			FVector V1 = FVector(matrixPerlin[z][y], float(y * gridSize), float(z * gridSize));
			FVector V2 = FVector(matrixPerlin[z][y + 1], float( (y + 1) * gridSize), float(z * gridSize));
			FVector V3 = FVector(matrixPerlin[z + 1][y], float(y * gridSize), float((z + 1) * gridSize));
			FVector V4 = FVector(matrixPerlin[z + 1][y + 1], float( (y + 1) * gridSize), float((z + 1) * gridSize));

			auto Normal = FVector::CrossProduct(V3 - V1, V4 - V1);
			auto Normal2 = FVector::CrossProduct(V2 - V4, V1 - V4);
			auto Color = FLinearColor::MakeRandomColor();

			Normal.Normalize();
			Normal2.Normalize();

			MeshData.Vertices.Append({ V1, V3, V4 });
			MeshData.Triangles.Append({
				VertexCount + 0,
				VertexCount + 1,
				VertexCount + 2,
				});
			MeshData.Normals.Append({
				Normal,
				Normal,
				Normal
				});
			MeshData.Colors.Append({
				Color,
				Color,
				Color
				});
			VertexCount += 3;

			MeshData.Vertices.Append({ V4, V2, V1 });
			MeshData.Triangles.Append({
				VertexCount + 0,
				VertexCount + 1,
				VertexCount + 2,
				});
			MeshData.Normals.Append({
				Normal2,
				Normal2,
				Normal2
				});
			MeshData.Colors.Append({
				Color,
				Color,
				Color
				});
			VertexCount += 3;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Mesh set"));
}

void ACavernGenerator::ApplyMesh() const
{
	Mesh->SetMaterial(0, Material);
	Mesh->CreateMeshSection_LinearColor(
		0,
		MeshData.Vertices,
		MeshData.Triangles,
		MeshData.Normals,
		MeshData.UV0,
		MeshData.Colors,
		TArray<FProcMeshTangent>(),
		true
	);
}


