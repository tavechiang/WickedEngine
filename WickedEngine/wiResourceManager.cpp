#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "wiSound.h"
#include "wiHelper.h"
#include "Utility/WicTextureLoader.h"
#include "Utility/DDSTextureLoader.h"

wiResourceManager::filetypes wiResourceManager::types;
wiResourceManager* wiResourceManager::globalResources = nullptr;

wiResourceManager::wiResourceManager()
{
	wiThreadSafeManager();
}
wiResourceManager::~wiResourceManager()
{
	CleanUp();
}
wiResourceManager* wiResourceManager::GetGlobal()
{
	if (globalResources == nullptr)
	{
		LOCK_STATIC();
		if (globalResources == nullptr)
		{
			globalResources = new wiResourceManager();
		}
		UNLOCK_STATIC();
	}
	return globalResources;
}
wiResourceManager* wiResourceManager::GetShaderManager()
{
	static wiResourceManager* shaderManager = new wiResourceManager;
	return shaderManager;
}

void wiResourceManager::SetUp()
{
	types.clear();
	types.insert( pair<string,Data_Type>("jpg",IMAGE) );
	types.insert( pair<string,Data_Type>("JPG",IMAGE) );
	types.insert( pair<string,Data_Type>("png",IMAGE) );
	types.insert( pair<string,Data_Type>("PNG",IMAGE) );
	types.insert( pair<string,Data_Type>("dds",IMAGE) );
	types.insert(pair<string, Data_Type>("wav", SOUND));
}

const wiResourceManager::Resource* wiResourceManager::get(const string& name)
{
	container::iterator it = resources.find(name);
	if(it!=resources.end())
		return it->second;
	else return nullptr;
}

void* wiResourceManager::add(const string& name, Data_Type newType
	, wiRenderer::VertexLayoutDesc* vertexLayoutDesc, UINT elementCount, D3D11_SO_DECLARATION_ENTRY* streamOutDecl)
{
	if (types.empty())
		SetUp();

	const Resource* res = get(name);
	if(!res){
		string ext = name.substr(name.length()-3,name.length());
		Data_Type type;
#pragma region dynamic type selection
		if(newType==Data_Type::DYNAMIC){
			filetypes::iterator it = types.find(ext);
			if(it!=types.end())
				type = it->second;
			else 
				return nullptr;
		}
		else 
			type = newType;
#pragma endregion
		void* success = nullptr;

		LOCK();

		switch(type){
		case Data_Type::IMAGE:
		{
			wiRenderer::TextureView image=nullptr;
			if(
					!ext.compare("jpg")
				|| !ext.compare("JPG")
				|| !ext.compare("png")
				|| !ext.compare("PNG")
				)
			{
				wiRenderer::graphicsMutex.lock();
				CreateWICTextureFromFile(true,wiRenderer::graphicsDevice,wiRenderer::getImmediateContext(),(wchar_t*)(wstring(name.begin(),name.end()).c_str()),nullptr,&image);
				wiRenderer::graphicsMutex.unlock();
			}
			else if(!ext.compare("dds")){
				CreateDDSTextureFromFile(wiRenderer::graphicsDevice,(wchar_t*)(wstring(name.begin(),name.end()).c_str()),nullptr,&image);
			}

			if(image)
				success=image;
		}
		break;
		case Data_Type::IMAGE_STAGING:
		{
			wiRenderer::APIResource image=nullptr;
			if(!ext.compare("dds")){
				CreateDDSTextureFromFileEx(wiRenderer::graphicsDevice,(wchar_t*)(wstring(name.begin(),name.end()).c_str()),0
					,D3D11_USAGE_STAGING,0,D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE,0,false
					,&image,nullptr);
			}

			if(image)
				success=image;
		}
		break;
		case Data_Type::SOUND:
		{
			success = new wiSoundEffect(name);
		}
		break;
		case Data_Type::MUSIC:
		{
			success = new wiMusic(name);
		}
		break;
		case Data_Type::VERTEXSHADER:
		{
			BYTE* buffer;
			size_t bufferSize;
			if (wiHelper::readByteData(name, &buffer, bufferSize)){
				wiRenderer::VertexShaderInfo* vertexShaderInfo = new wiRenderer::VertexShaderInfo;
				wiRenderer::graphicsDevice->CreateVertexShader(buffer, bufferSize, NULL, &vertexShaderInfo->vertexShader);
				if (vertexLayoutDesc != nullptr && elementCount > 0){
					wiRenderer::graphicsDevice->CreateInputLayout(vertexLayoutDesc, elementCount, buffer, bufferSize, &vertexShaderInfo->vertexLayout);
					if (vertexShaderInfo->vertexLayout == nullptr)
					{
						success = nullptr;
					}
					else
					{
						success = vertexShaderInfo;
					}
				}
				else
				{
					success = (vertexShaderInfo->vertexShader == nullptr ? nullptr : vertexShaderInfo);
				}
				delete[] buffer;
			}
			else{
				success = nullptr;
			}
		}
		break;
		case Data_Type::PIXELSHADER:
		{
			BYTE* buffer;
			size_t bufferSize;
			if (wiHelper::readByteData(name, &buffer, bufferSize)){
				wiRenderer::PixelShader shader = nullptr;
				wiRenderer::graphicsDevice->CreatePixelShader(buffer, bufferSize, nullptr, &shader);
				delete[] buffer;
				success = shader;
			}
			else{
				success = nullptr;
			}
		}
		break;
		case Data_Type::GEOMETRYSHADER:
		{
			BYTE* buffer;
			size_t bufferSize;
			if (wiHelper::readByteData(name, &buffer, bufferSize)){
				wiRenderer::GeometryShader shader = nullptr;
				wiRenderer::graphicsDevice->CreateGeometryShader(buffer, bufferSize, nullptr, &shader);
				if (streamOutDecl != nullptr && elementCount > 0){
					wiRenderer::graphicsDevice->CreateGeometryShaderWithStreamOutput(buffer, bufferSize, streamOutDecl,
						elementCount, NULL, 0, shader ? 0 : D3D11_SO_NO_RASTERIZED_STREAM, NULL, &shader);
				}
				delete[] buffer;
				success = shader;
			}
			else{
				success = nullptr;
			}
		}
		break;
		case Data_Type::HULLSHADER:
		{
			BYTE* buffer;
			size_t bufferSize;
			if (wiHelper::readByteData(name, &buffer, bufferSize)){
				wiRenderer::HullShader shader = nullptr;
				wiRenderer::graphicsDevice->CreateHullShader(buffer, bufferSize, nullptr, &shader);
				delete[] buffer;
				success = shader;
			}
			else{
				success = nullptr;
			}
		}
		break;
		case Data_Type::DOMAINSHADER:
		{
			BYTE* buffer;
			size_t bufferSize;
			if (wiHelper::readByteData(name, &buffer, bufferSize)){
				wiRenderer::DomainShader shader = nullptr;
				wiRenderer::graphicsDevice->CreateDomainShader(buffer, bufferSize, nullptr, &shader);
				delete[] buffer;
				success = shader;
			}
			else{
				success = nullptr;
			}
		}
		break;
		case Data_Type::COMPUTESHADER:
		{
			//TODO
			success = nullptr;
		}
		break;
		default:
			success=nullptr;
			break;
		};

		if(success)
			resources.insert( pair<string,Resource*>(name,new Resource(success,type)) );

		UNLOCK();

		return success;
	}
	return res->data;
}

bool wiResourceManager::del(const string& name)
{
	Resource* res = nullptr;
	container::iterator it = resources.find(name);
	if (it != resources.end())
		res = it->second;
	else return false;

	if(res){
		LOCK();
		bool success = true;

		if(res->data)
			switch(res->type){
			case Data_Type::IMAGE:
			case Data_Type::IMAGE_STAGING:
				wiRenderer::SafeRelease(reinterpret_cast<wiRenderer::TextureView&>(res->data));
				break;
			case Data_Type::VERTEXSHADER:
				{
					wiRenderer::VertexShaderInfo* vsinfo = (wiRenderer::VertexShaderInfo*)res->data;
					wiRenderer::SafeRelease(vsinfo->vertexLayout);
					wiRenderer::SafeRelease(vsinfo->vertexShader);
					delete vsinfo;
				}
				break;
			case Data_Type::PIXELSHADER:
				wiRenderer::SafeRelease(reinterpret_cast<wiRenderer::PixelShader&>(res->data));
				break;
			case Data_Type::GEOMETRYSHADER:
				wiRenderer::SafeRelease(reinterpret_cast<wiRenderer::GeometryShader&>(res->data));
				break;
			case Data_Type::HULLSHADER:
				wiRenderer::SafeRelease(reinterpret_cast<wiRenderer::HullShader&>(res->data));
				break;
			case Data_Type::DOMAINSHADER:
				wiRenderer::SafeRelease(reinterpret_cast<wiRenderer::DomainShader&>(res->data));
				break;
			case Data_Type::COMPUTESHADER:
				wiRenderer::SafeRelease(reinterpret_cast<wiRenderer::ComputeShader&>(res->data));
				break;
			case Data_Type::SOUND:
			case Data_Type::MUSIC:
				delete ((wiSound*)res->data);
				break;
			default:
				success=false;
				break;
			};

		delete res;
		resources.erase(name);

		UNLOCK();

		return success;
	}
	return false;
}

bool wiResourceManager::CleanUp()
{
	vector<string>resNames(0);
	for (auto& x : resources)
	{
		resNames.push_back(x.first);
	}
	for (auto& x : resNames)
	{
		del(x);
	}
	LOCK();
	resources.clear();
	UNLOCK();
	return true;
}
