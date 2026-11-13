#include "Interfaces/IPluginManager.h"
#include "MeshDescription.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "PhysicsEngine/ConvexElem.h"
#include "QuakeConversion.h"
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
