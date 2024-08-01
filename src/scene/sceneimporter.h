class Scene;
class Mesh;
class Transform;
class Material;

struct aiScene;
struct aiMesh;
struct aiNode;
struct aiMaterial;

#include <vector>
#include <string>

class SceneImporter
{
public:
	static Scene *import(std::string filename);

private:
	SceneImporter(const aiScene *source);

	std::vector<Mesh*> meshes;
	std::vector<Material*> materials;

	Mesh *convertMesh(const aiMesh *mesh);
	void convertMeshes();

	Material *convertMaterial(const aiMaterial *material);
	void convertMaterials();

	void traverseChildren(const aiNode *node, Transform *parentTransform);
	void traverseNode(const aiNode *node, Transform *parentTransform);

	const aiScene *source;
	Scene *result;
};
