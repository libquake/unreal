#include "Interfaces/IPluginManager.h"
#include "MeshDescription.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "PhysicsEngine/ConvexElem.h"
#include "QuakeConversion.h"
#include "QuakeEntityComponent.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshOperations.h"

namespace
{
	bool BuildTestMap(FAutomationTestBase& Test, quakelib::Level& OutLevel)
	{
		const FString Data = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("LibQuake"))->GetBaseDir(),
			TEXT("Tests"), TEXT("Data"));
		FString Error;
		quakelib::MapDocument Document;
		if (!LibQuake::ParseMap(Data / TEXT("sky_alpha.map"), Document, Error) ||
			!LibQuake::BuildMap(Document, {}, OutLevel, Error))
		{
			Test.AddError(Error);
			return false;
		}
		return true;
	}

	const quakelib::EntityGeometry* FindGeometry(const quakelib::Level& Level, const char* ClassName)
	{
		for (const quakelib::LevelEntity& Entity : Level.entities)
		{
			if (Entity.classname == ClassName && Entity.geometry >= 0)
			{
				return &Level.geometry[Entity.geometry];
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuakeMeshDescriptionTest, "LibQuake.Conversion.MeshDescription",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FQuakeMeshDescriptionTest::RunTest(const FString& Parameters)
{
	quakelib::Level Level;
	if (!BuildTestMap(*this, Level))
	{
		return false;
	}
	const quakelib::EntityGeometry* World = FindGeometry(Level, "worldspawn");
	if (!TestNotNull(TEXT("worldspawn"), World))
	{
		return false;
	}

	FMeshDescription Mesh;
	TestTrue(TEXT("worldspawn is visible"), LibQuake::BuildMeshDescription(*World, LibQuake::DefaultScale, Mesh));

	FStaticMeshAttributes Attributes(Mesh);
	TestEqual(TEXT("texture and lightmap uvs"), Attributes.GetVertexInstanceUVs().GetNumChannels(), 2);
	TestTrue(TEXT("a polygon group per texture"), Mesh.PolygonGroups().Num() > 1);

	// Unreal derives the front face from the winding, it has to agree with the normals libquake computed
	FStaticMeshOperations::ComputeTriangleTangentsAndNormals(Mesh);
	const TTriangleAttributesConstRef<FVector3f> FromWinding = Attributes.GetTriangleNormals();
	const TVertexInstanceAttributesConstRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();
	int32 Flipped = 0;
	for (const FTriangleID Triangle : Mesh.Triangles().GetElementIDs())
	{
		const FVertexInstanceID Corner = Mesh.GetTriangleVertexInstances(Triangle)[0];
		if (FVector3f::DotProduct(FromWinding[Triangle], Normals[Corner]) < 0)
		{
			Flipped++;
		}
	}
	TestEqual(TEXT("no triangle faces the wrong way"), Flipped, 0);

	const FBox Bounds = Mesh.ComputeBoundingBox();
	TestTrue(TEXT("scaled to centimeters"), Bounds.GetSize().X > 100 && Bounds.GetSize().X < 10000);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuakeConvexElementsTest, "LibQuake.Conversion.ConvexElements",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FQuakeConvexElementsTest::RunTest(const FString& Parameters)
{
	quakelib::Level Level;
	if (!BuildTestMap(*this, Level))
	{
		return false;
	}
	const quakelib::EntityGeometry* World = FindGeometry(Level, "worldspawn");
	if (!TestNotNull(TEXT("worldspawn"), World))
	{
		return false;
	}

	const TArray<FKConvexElem> All = LibQuake::BuildConvexElements(*World, LibQuake::DefaultScale,
		[](quakelib::CollisionHull::Type) { return true; });
	TestEqual(TEXT("one element per hull"), All.Num(), (int32)World->hulls.size());
	for (const FKConvexElem& Element : All)
	{
		TestTrue(TEXT("hull has volume"), Element.ElemBox.GetVolume() > 0);
	}

	const TArray<FKConvexElem> None = LibQuake::BuildConvexElements(*World, LibQuake::DefaultScale,
		[](quakelib::CollisionHull::Type) { return false; });
	TestEqual(TEXT("filter is applied"), None.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuakeAnglesTest, "LibQuake.Conversion.Angles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FQuakeAnglesTest::RunTest(const FString& Parameters)
{
	UQuakeEntityComponent* Entity = NewObject<UQuakeEntityComponent>();
	const auto Direction = [Entity](const TCHAR* Key, const TCHAR* Value)
	{
		Entity->Keys.Reset();
		Entity->Keys.Add(Key, Value);
		return Entity->GetMoveDirection();
	};

	// Quake's +y is Unreal's -y
	TestTrue(TEXT("angle 90 moves to quake's +y"), Direction(TEXT("angle"), TEXT("90")).Equals(FVector(0, -1, 0)));
	TestTrue(TEXT("angle -1 moves up"), Direction(TEXT("angle"), TEXT("-1")).Equals(FVector::UpVector));
	TestTrue(TEXT("angle -2 moves down"), Direction(TEXT("angle"), TEXT("-2")).Equals(FVector::DownVector));
	TestTrue(TEXT("a positive pitch looks down"),
		Direction(TEXT("angles"), TEXT("45 0 0")).Equals(FVector(1, 0, -1).GetSafeNormal(), 0.001));
	TestTrue(TEXT("without a key it points along x"), Direction(TEXT("other"), TEXT("1")).Equals(FVector(1, 0, 0)));

	Entity->Keys.Reset();
	Entity->Keys.Add(TEXT("angles"), TEXT("10 20 30"));
	TestEqual(TEXT("angles turn all three"), Entity->GetRotation(), FRotator(-10, -20, -30));
	return true;
}
