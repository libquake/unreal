#include "QuakeConversion.h"

#include "MeshDescription.h"
#include "PhysicsEngine/ConvexElem.h"
#include "StaticMeshAttributes.h"

namespace LibQuake
{
	bool BuildMeshDescription(const quakelib::EntityGeometry& Geometry, float Scale, FMeshDescription& OutMesh)
	{
		FStaticMeshAttributes Attributes(OutMesh);
		Attributes.Register();
		TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector2f> UVs = Attributes.GetVertexInstanceUVs();
		TPolygonGroupAttributesRef<FName> SlotNames = Attributes.GetPolygonGroupMaterialSlotNames();
		UVs.SetNumChannels(2);

		// faces of neighbouring brushes share corners, one vertex per position keeps the mesh connected
		TMap<FVector3f, FVertexID> Welded;
		TArray<FVertexInstanceID> Instances;
		for (const quakelib::RenderMesh& Mesh : Geometry.meshes)
		{
			if (Mesh.type != quakelib::SurfaceType::SOLID || Mesh.indices.empty())
			{
				continue;
			}

			const FPolygonGroupID Group = OutMesh.CreatePolygonGroup();
			SlotNames[Group] = FName(UTF8_TO_TCHAR(Mesh.textureName.c_str()));

			Instances.Reset(Mesh.vertices.size());
			for (const quakelib::Vertex& Vertex : Mesh.vertices)
			{
				const FVector3f Position = ToUnrealPosition(Vertex.point, Scale);
				FVertexID* Shared = Welded.Find(Position);
				const FVertexID VertexID = Shared ? *Shared : Welded.Add(Position, OutMesh.CreateVertex());
				Positions[VertexID] = Position;

				const FVertexInstanceID Instance = OutMesh.CreateVertexInstance(VertexID);
				Normals[Instance] = ToUnrealDirection(Vertex.normal);
				UVs.Set(Instance, 0, FVector2f(Vertex.uv.X, Vertex.uv.Y));
				UVs.Set(Instance, 1, FVector2f(Vertex.lightmap_uv.X, Vertex.lightmap_uv.Y));
				Instances.Add(Instance);
			}

			// the mirror turns libquake's front faces around, the second and third corner swap places
			for (size_t i = 0; i + 2 < Mesh.indices.size(); i += 3)
			{
				const FVertexInstanceID Triangle[] = {
					Instances[Mesh.indices[i]], Instances[Mesh.indices[i + 2]], Instances[Mesh.indices[i + 1]]};
				OutMesh.CreateTriangle(Group, Triangle);
			}
		}
		return OutMesh.Triangles().Num() > 0;
	}

	bool BuildHullMeshDescription(const quakelib::EntityGeometry& Geometry, float Scale, FMeshDescription& OutMesh)
	{
		FStaticMeshAttributes Attributes(OutMesh);
		Attributes.Register();
		TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
		const FPolygonGroupID Group = OutMesh.CreatePolygonGroup();
		Attributes.GetPolygonGroupMaterialSlotNames()[Group] = TEXT("hull");

		TArray<FVertexInstanceID> Instances;
		for (const quakelib::CollisionHull& Hull : Geometry.hulls)
		{
			Instances.Reset(Hull.points.size());
			for (const quakelib::Vec3& Point : Hull.points)
			{
				const FVertexID Vertex = OutMesh.CreateVertex();
				Positions[Vertex] = ToUnrealPosition(Point, Scale);
				Instances.Add(OutMesh.CreateVertexInstance(Vertex));
			}
			// mirrored like the render meshes
			for (size_t i = 0; i + 2 < Hull.indices.size(); i += 3)
			{
				const FVertexInstanceID Triangle[] = {
					Instances[Hull.indices[i]], Instances[Hull.indices[i + 2]], Instances[Hull.indices[i + 1]]};
				OutMesh.CreateTriangle(Group, Triangle);
			}
		}
		return OutMesh.Triangles().Num() > 0;
	}

	TArray<FKConvexElem> BuildConvexElements(const quakelib::EntityGeometry& Geometry, float Scale,
		TFunctionRef<bool(quakelib::CollisionHull::Type)> Include)
	{
		TArray<FKConvexElem> Elements;
		for (const quakelib::CollisionHull& Hull : Geometry.hulls)
		{
			if (!Include(Hull.type) || Hull.points.size() < 4)
			{
				continue;
			}
			FKConvexElem& Element = Elements.AddDefaulted_GetRef();
			Element.VertexData.Reserve(Hull.points.size());
			for (const quakelib::Vec3& Point : Hull.points)
			{
				Element.VertexData.Add(FVector(ToUnrealPosition(Point, Scale)));
			}
			Element.UpdateElemBox();
		}
		return Elements;
	}
}
