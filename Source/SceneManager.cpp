///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	bool bReturn = false;

	bReturn = CreateGLTexture("./textures/Porcelain.jpg", "cup"); //load texture from file to texture array

	bReturn = CreateGLTexture("./textures/Wood1.jpg", "table-top");

	bReturn = CreateGLTexture("./textures/Wood2.png", "table-side");

	bReturn = CreateGLTexture("./textures/Metal.jpg", "base");

	bReturn = CreateGLTexture("./textures/Wood3.jpg", "WoodFloor");

	bReturn = CreateGLTexture("./textures/BlackSteel.png", "lamp-rim");

	bReturn = CreateGLTexture("./textures/WhiteCloth.jpg", "lamp-shade");

	bReturn = CreateGLTexture("./textures/Wall.jpg", "wall");

	bReturn = CreateGLTexture("./textures/WhitePlastic.jpg", "plastic");

	bReturn = CreateGLTexture("./textures/BookSpine.png", "BookSpine");

	bReturn = CreateGLTexture("./textures/BookBack.png", "BookBack");

	bReturn = CreateGLTexture("./textures/BookFront.png", "BookFront");

	bReturn = CreateGLTexture("./textures/Pages.png", "Pages");






	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/
	OBJECT_MATERIAL plasticMaterial;
	//Set the color emitted from the object when light interacts with it
	plasticMaterial.diffuseColor = glm::vec3(0.9f, 0.9f, 0.9f);
	//Specular color is set to a lower value that minimize reflection and emits a gray color
	plasticMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	//shininess effects how large or small the specular colors will appear
	plasticMaterial.shininess = 1.0;
	//create a tag to be used for the assignment of material for objects in RenderScene()
	plasticMaterial.tag = "plastic";

	m_objectMaterials.push_back(plasticMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 5.0;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL woodFloorMaterial;
	//Increase intensity of diffuse to give a lighter tone in relation to the desk wood
	woodFloorMaterial.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	//Keep wood floor at lower specularity to match reference image
	woodFloorMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	//Lower Levels of shininess match floor in reference image
	woodFloorMaterial.shininess = 0.2;
	woodFloorMaterial.tag = "woodFloor";

	m_objectMaterials.push_back(woodFloorMaterial);

	OBJECT_MATERIAL porcelainMaterial;
	porcelainMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f); // Light off-white color
	porcelainMaterial.specularColor = glm::vec3(0.8f, 0.8f, 0.9f); // Slight blue specular reflection
	porcelainMaterial.shininess = 32.0f; // Higher shininess for porcelain
	porcelainMaterial.tag = "porcelain";

	m_objectMaterials.push_back(porcelainMaterial);


	OBJECT_MATERIAL metalMaterial;
	metalMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	metalMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.8f);
	metalMaterial.shininess = 8.0;
	metalMaterial.tag = "metal";

	m_objectMaterials.push_back(metalMaterial);

	OBJECT_MATERIAL clothMaterial;
	// Higher diffuse color for higher impact from lighting, giving effect of translucency
	clothMaterial.diffuseColor = glm::vec3(10.0f, 10.0f, 10.0f);
	// Low specular reflection to mimic cloth
	clothMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	// Lower shininess for a diffuse look
	clothMaterial.shininess = 2.0f;
	clothMaterial.tag = "cloth";

	m_objectMaterials.push_back(clothMaterial);

	OBJECT_MATERIAL wallMaterial;
	// Soft white lower diffuse to give more realistic light affect on wall material
	wallMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	// Low specular reflection to mimic drywall
	wallMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	// Lower shininess for a diffuse look
	wallMaterial.shininess = 1.0f;
	wallMaterial.tag = "wall";

	m_objectMaterials.push_back(wallMaterial);

	OBJECT_MATERIAL bookCoverMaterial;
	// A dark diffuse color to represent the base color of the book cover
	bookCoverMaterial.diffuseColor = glm::vec3(0.5f, 0.1f, 0.1f);
	bookCoverMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	bookCoverMaterial.shininess = 16.0f;
	// Tag for identifying the material when rendering
	bookCoverMaterial.tag = "BookCover";

	// Add the material to the collection
	m_objectMaterials.push_back(bookCoverMaterial);
}

/****************************************************************/
/*****************Generate Lamp**********************************/
/****************************************************************/
void SceneManager::LoadLamp(float xPosition = 0.0f, float yPosition = 0.0f, float zPosition = 0.0f) {

	

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	//Lamp Shade
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.75f, 1.0f, 0.75f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.2f + xPosition, 5.3f + yPosition, -1.6f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);//Increase texture tiling for legs as they are longer
	//Keep texture tiling duplication on the same axis for legs due to rotation of object still matching length
	//Set Lamp Shade texture to white cloth
	SetShaderTexture("lamp-shade");
	//Set Shader Material for light interaction
	SetShaderMaterial("cloth");
	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh(false, false, true);
	/****************************************************************/

	//metal shade rim top
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.35f, 0.35f, 0.1f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.20f + xPosition, 6.3f + yPosition, -1.6f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);//Increase texture tiling for legs as they are longer
	//Keep texture tiling duplication on the same axis for legs due to rotation of object still matching length
	SetShaderTexture("lamp-rim");
	//Set Shader Material to reflect lamp rim
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/

	//metal shade rim bottom
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.65f, 0.65f, 0.1f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.20f + xPosition, 5.3f + yPosition, -1.6f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);//Increase texture tiling for legs as they are longer
	//Keep texture tiling duplication on the same axis for legs due to rotation of object still matching length
	SetShaderTexture("lamp-rim");
	//Set Shader Material to reflect lamp rim bottom
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();


	/****************************************************************/

	//metal lamp base
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.05f, .75f, 0.05f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.20f + xPosition, 4.85f + yPosition, -1.6f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);//Increase texture tiling for legs as they are longer
	//Keep texture tiling duplication on the same axis for legs due to rotation of object still matching length
	SetShaderTexture("lamp-rim");
	//Set Shader Material for light interaction
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//metal lamp leg Front
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.025f, 1.0f, 0.025f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 160.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.20f + xPosition, 4.85f + yPosition, -1.6f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);//Increase texture tiling for legs as they are longer
	//Keep texture tiling duplication on the same axis for legs due to rotation of object still matching length
	SetShaderTexture("lamp-rim");
	//Set Shader Material for light interaction
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/

	//metal lamp leg Front
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.025f, 1.0f, 0.025f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 160.0f;
	YrotationDegrees = 120.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.20f + xPosition, 4.85f + yPosition, -1.6f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);//Increase texture tiling for legs as they are longer
	//Keep texture tiling duplication on the same axis for legs due to rotation of object still matching length
	SetShaderTexture("lamp-rim");
	//Set Shader Material for light interaction
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/

	//metal lamp leg Front
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.025f, 1.0f, 0.025f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 160.0f;
	YrotationDegrees = 240.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.20f + xPosition, 4.85f + yPosition, -1.6f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);//Increase texture tiling for legs as they are longer
	//Keep texture tiling duplication on the same axis for legs due to rotation of object still matching length
	SetShaderTexture("lamp-rim");
	//Set Shader Material for light interaction
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/

}
/****************************************************************/
/*****************Generate Monitor**********************************/
/****************************************************************/

void SceneManager::LoadMonitor(float xPosition = 0.0f, float yPosition = 0.0f, float zPosition = 0.0f) {


	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	//Center of monitor
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 2.0f, 0.25f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + xPosition, 4.068f + yPosition, 5.005f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic 
	SetShaderColor(0.1,0.1,0.1,1);
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

	//Bottom of monitor
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, .25f, 0.25f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f+ xPosition, 3.0f + yPosition, 5.0f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic 
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	//Top part of monitor
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 0.25f, 0.25f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + xPosition, 5.0f + yPosition, 5.0f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic 
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	
	//Right part of monitor
	
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.25f, 1.9f, 0.25f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.5f + xPosition, 4.0f + yPosition, 5.0f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic 
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	
	//Left part of monitor
	
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.25f, 1.9f, 0.25f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.49f + xPosition, 4.0f + yPosition, 5.0f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic 
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	//Top Left Corner part of monitor

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.15f, 0.25f, 0.25f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.47f + xPosition, 4.878f + yPosition, 4.875f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic 
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	
	/****************************************************************/

	//Bottom Left Corner part of monitor

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.15f, 0.25f, 0.25f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.47f + xPosition, 3.12f + yPosition, 4.87f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic 
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	//Bottom Right Corner part of monitor

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.15f, 0.25f, 0.25f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.4825f + xPosition, 3.125f + yPosition, 4.87f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic 
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	//Top Right Corner part of monitor

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.15f, 0.25f, 0.25f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.4825f + xPosition, 4.878f + yPosition, 4.87f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic 
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	//Bottom part of monitor stand

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.05f, 1.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + xPosition, 2.55f + yPosition, 5.0f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic 
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	
	/****************************************************************/

	//vertical part of monitor stand

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.05f, 1.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = -65.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + xPosition, 3.025f + yPosition, 4.735f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic 
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	//Cylinder attatchment to both parts of monitor stand

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 1.0f, .03f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.5f + xPosition, 2.575f + yPosition, 4.53f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic 
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//Plane to act as back panel of monitor

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.55f, 1.2f, 1.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + xPosition, 4.0f + yPosition, 4.855f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic 
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
}

/****************************************************************/
/*****************Generate Keyboard*******************************/
/****************************************************************/
void SceneManager::LoadKeyboard(float xPosition = 0.0f, float yPosition = 0.0f, float zPosition = 0.0f) {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	//Base of Keyboard
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, .5f, 0.05f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = -80.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + xPosition, 4.068f + yPosition, 5.005f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic 
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//*********Generate All Keycaps**********************************/
	//**************************************************************/
	float keySpace = 0;
	float yMod = 0;
	float zMod = 0;
	for (int j = 0; j < 3; j++) {
		for (int i = 0; i < 19; i++) {
			GenerateKeyCap(xPosition + keySpace, yPosition+yMod, zPosition+zMod);
			keySpace += .1;
		}
		keySpace = 0;
		yMod -= .025;
		zMod += .15;
	}

	/**********************************************************************/
	//Keyboard stand
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, 0.15f, 0.005f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = -120.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + xPosition, 4.068f + yPosition, 4.8f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic 
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	




}

void SceneManager::GenerateKeyCap(float xPosition = 0.0f, float yPosition = 0.0f, float zPosition = 0.0f) {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	//Base of Keyboard
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.075f, 0.075f, 0.025f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = -80.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-.9f + xPosition, 4.12f + yPosition, 4.85f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic 
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

}


/****************************************************************/
/*****************Generate Mouse*******************************/
/****************************************************************/
void SceneManager::LoadMouse(float xPosition = 0.0f, float yPosition = 0.0f, float zPosition = 0.0f) {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	//Base of Mouse
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.15f, 0.25f, 0.1f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-.9f + xPosition, 4.12f + yPosition, 4.85f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic 
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	/****************************************************************/
	//Mouse Wheel
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.1f, 0.08f, 0.2f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-.9f + xPosition, 4.14f + yPosition, 4.75f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic 
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfTorusMesh();

	/****************************************************************/
	
}


/****************************************************************/
/*****************Generate Phone*******************************/
/****************************************************************/
void SceneManager::LoadPhone(float xPosition = 0.0f, float yPosition = 0.0f, float zPosition = 0.0f) {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	//Left Part of Phone
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 0.03f, 0.5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-.9f + xPosition, 4.12f + yPosition, 4.85f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//Right part of Phone
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 0.03f, 0.5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-.5f + xPosition, 4.12f + yPosition, 4.85f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//Top part of Phone
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.03f, 0.1f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-.7f + xPosition, 4.12f + yPosition, 4.585f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	
	//Bottom Part of Phone
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.03f, 0.1f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-.7f + xPosition, 4.12f + yPosition, 5.1f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	

	//Bottom Left part of Phone
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.05f, 0.03f, 0.05f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-.9f + xPosition, 4.10f + yPosition, 5.1f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/


	//Top Left part of Phone
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.05f, 0.03f, 0.05f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-.9010f + xPosition, 4.10f + yPosition, 4.5875f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/


	//Top Right part of Phone
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.05f, 0.03f, 0.05f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-.50 + xPosition, 4.10f + yPosition, 4.59f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/


	//Bottom Right part of Phone
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.05f, 0.03f, 0.05f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-.50 + xPosition, 4.10f + yPosition, 5.1f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic
	SetShaderTexture("plastic");
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/


	//Screen of Phone
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.025f, 0.55f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-.70 + xPosition, 4.125f + yPosition, 4.85f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(4, 1);
	//Set Shader Texture to plastic
	SetShaderColor(.2, .2, .2, 1);
	//Set Shader Material for light interaction
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

}

/****************************************************************/
/*****************Generate Book*******************************/
/****************************************************************/
void SceneManager::LoadBook(
	float xPosition = 0.0f,
	float yPosition = 0.0f,
	float zPosition = 0.0f,
	float xRotation = 0.0f,
	float yRotation = 0.0f,
	float zRotation = 0.0f) {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	//Left book cover
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.05f, 1.2f, 1.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f + xRotation;
	YrotationDegrees = 0.0f + yRotation;
	ZrotationDegrees = 0.0f + zRotation;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-.2f + xPosition, 4.3f + yPosition, 4.85f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(1, 1);
	SetShaderTexture("BookBack");
	//Set Shader Material for light interaction
	SetShaderMaterial("BookCover");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/


	//right book cover
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.05f, 1.2f, 1.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f + xRotation;
	YrotationDegrees = 0.0f + yRotation;
	ZrotationDegrees = 0.0f + zRotation;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + xPosition, 4.3f + yPosition, 4.85f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(1, 1);
	SetShaderTexture("BookFront");
	//Set Shader Material for light interaction
	SetShaderMaterial("BookCover");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

	//Book Spine
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.25f, 1.2f, .05f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f + xRotation;
	YrotationDegrees = 0.0f + yRotation;
	ZrotationDegrees = 0.0f + zRotation;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.1025f + xPosition, 4.3f + yPosition, 5.330f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(1, 1);
	SetShaderTexture("BookSpine");
	//Set Shader Material for light interaction
	SetShaderMaterial("BookCover");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/


	//Book Pages
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.17f, 1.1f, 0.95f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f + xRotation;
	YrotationDegrees = 0.0f + yRotation;
	ZrotationDegrees = 0.0f + zRotation;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.09f + xPosition, 4.3f + yPosition, 4.85f + zPosition);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTextureUVScale(1, 1);
	SetShaderTexture("Pages");
	//Set Shader Material for light interaction
	SetShaderMaterial("BookCover");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

}


/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/

	// First Point white light emitting from 10 units above and 20 units closer to camera spawn to emulate a room light in the center of a room
	m_pShaderManager->setVec3Value("pointLights[0].position", 0.0f, 20.0f, 20.0f);
	//moderate levels of ambient to imitate a room light
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.1f, 0.1f, 0.1f);
	//higher intensity diffuse for direct impact on the scene
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.4f, 0.4f, 0.4f);
	//lower levels of specular white light causing less impact on reflections
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	//************************************************************************************************//
	//****Position Lights located at 2 locations around the lamp to give impression of translucency***//
	//************************************************************************************************//
	
	//Position light behind lamp
	m_pShaderManager->setVec3Value("pointLights[1].position", -4.0f, 5.5f, -2.5f);
	//moderate levels of ambient to imitate a lamp light
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.4f, 0.4f, 0.4f);
	//moderate diffuse to ensure diffussion of light is accurately reflected dependent on material
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.5f, 0.5f, 0.5f);
	//lower levels of specular white light causing less impact on reflections
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	//Position light to the left of the lamp
	m_pShaderManager->setVec3Value("pointLights[2].position", -6.0f, 5.5f, -1.5f);
	//Ambient light at moderate levels to immitate lamp light
	m_pShaderManager->setVec3Value("pointLights[2].ambient", 0.4f, 0.4f, 0.4f);
	//moderate diffuse to ensure diffussion of light is accurately reflected dependent on material
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", 0.5f, 0.5f, 0.5f);
	//lower levels of specular white light causing less impact on reflections
	m_pShaderManager->setVec3Value("pointLights[2].specular", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setBoolValue("pointLights[2].bActive", true);
}
/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// Load textures once during preparation
	LoadSceneTextures();
	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	//Set Texture for lower plane
	SetShaderTexture("WoodFloor");
	SetTextureUVScale(4, 2);
	//Set Shader Material for light interaction
	SetShaderMaterial("woodFloor");
	
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
		// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 5.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 5.0f, -3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("wall");
	SetShaderMaterial("wall");
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	//Draw Coffee Cup Base
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.25f, 0.5f, .25f);//Tapered cylinder needs to be reduced by 50percent of it's base size in depth and width to better reflect the base of the coffee cup.

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;//Rotate Tapered Cylinder to be "upright" reflecting the base of the coffee cup
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.75f, 4.45f, 1.0f);//After rotating the tapered cylinder the base of the cup needs to be uplifted 4 units on the Y axis

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	
	SetShaderTexture("cup");//Set Cup texture to loaded texture tag
	SetTextureUVScale(2, 2);//increase tiling to give a more detailed appearance
	//Set Shader Material for light interaction
	SetShaderMaterial("porcelain");
	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh(true, false, true);
	

	/****************************************************************/
	//Draw Coffee Cup Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.13f, .15f, .13f);//Reduce size of the Torus to reflect coffee cup handle, reducing the scaling factor of the y plane by less than the others to give a slightly elongated vertical handle to reflect the picture

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = -8.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.75f, 4.22f, 1.17f);//set position for the mesh to halfway exist within the tapered cylinder to give the effect of a single object

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("cup");
	SetTextureUVScale(2, 2);
	//Set Shader Material for light interaction
	SetShaderMaterial("porcelain");
	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfTorusMesh();

	/****************************************************************/
	/*****************Generate Desk**********************************/
	/****************************************************************/
	///Draw Desk Tabletop
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(10.0f, .5f, 5.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-.4f, 3.65f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set Shader Material for light interaction
	SetShaderMaterial("wood");
	//Set Shader for the table top
	SetShaderTexture("table-top");
	//Set the tiling for greater visual detail and coherence to the object
	SetTextureUVScale(1, .75);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::top);//Pass through ShapeMeshes argument to apply texture to the top of the box object
	//Set texture for the sides to further match the texture differences in the reference image
	SetShaderTexture("table-side");
	//keep scaling 1:1 with the image 
	SetTextureUVScale(2, .25);//Change tiling to occur less often on the shorter sides with height modified for greater texture clarity
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::left);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::right);
	SetTextureUVScale(4, .25);//Change tiling to occur more often on the longer sides
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::back);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::front);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::bottom);
	/****************************************************************/
	//Draw the steal base underneath the wooden desk tabletop
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(9.5f, .5f, 4.5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-.4f, 3.2f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set texture to metal for base of the table
	SetShaderTexture("base");
	//Set Shader Material for light interaction
	SetShaderMaterial("metal");

	SetTextureUVScale(4, 1);//Set Tiling to occur more often due to length of the object, reduce stretching
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::back);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::front);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::bottom);
	
	SetTextureUVScale(2, .5);//Set Tiling to occur less often due to length of the object on sides, increase clarity
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::left);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::right);

	
	/****************************************************************/
	//Draw Left Lower connecting beam
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.5f, .5f, .25f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.025f, .75f, 0.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	
	
	SetTextureUVScale(2, 1);//decrease tiling for connecting beams as they are shorter
	//Set Shader Material for light interaction
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	
	/****************************************************************/
	//Draw right lower connecting beam
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.5f, .5f, .25f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.21f, 0.75f, 0.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	
	SetTextureUVScale(2, 1);//decrease tiling for connecting beams as they are shorter
	//Set Shader Material for light interaction
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//Back Right Leg
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, .25f, .5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.21f, 1.52f, -2.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	
	SetTextureUVScale(4, 1);//Increase texture tiling for legs as they are longer
	//Keep texture tiling duplication on the same axis for legs due to rotation of object still matching length
	//Set Shader Material for light interaction
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	//Front Right Leg
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, .25f, .5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.21f, 1.52f, 2.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	
	SetTextureUVScale(4, 1);//Increase texture tiling for legs as they are longer
	//Keep texture tiling duplication on the same axis for legs due to rotation of object still matching length
	//Set Shader Material for light interaction
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	//Front Left Leg
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, .25f, .5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.025f, 1.52f, 2.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	
	SetTextureUVScale(4, 1);//Increase texture tiling for legs as they are longer
	//Keep texture tiling duplication on the same axis for legs due to rotation of object still matching length
	//Set Shader Material for light interaction
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	//Back Left Leg
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, .25f, .5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.025f, 1.52f, -2.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	
	SetTextureUVScale(4, 1);//Increase texture tiling for legs as they are longer
	//Keep texture tiling duplication on the same axis for legs due to rotation of object still matching length
	//Set Shader Material for light interaction
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	//Load Lamp in a little to the left and back to match the scene
	LoadLamp(0.25, 0, -.2);
	
	//load in the monitor
	LoadMonitor(-.5, 1.4, -6.5);

	//Load in Keyboard
	LoadKeyboard(-.5, 0, -3.5);

	//Load in Mouse
	LoadMouse(2, -0.1, -3.4);

	//Load in Phone
	LoadPhone(2.7, 0, -3.9);

	//Load in Books
	float BookSpacer = 2.0;
	for (int i = 0; i < 8; i++){
		LoadBook(BookSpacer, .25, -6.35);
		BookSpacer += .25;
	}

	LoadBook(1.68, .25, -6.35, 0, 0, -7.5);

	
}
