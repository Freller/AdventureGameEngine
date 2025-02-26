#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "happly.h"

#include "mesh.h"


//find which triangles  form quads if they are 90deg walls
std::map<int, int> findQuadFaces(const std::vector<vertex3d>& vertices, const std::vector<face>& faces) {
    // Map to hold pairs of face indices with matching vertices
    std::map<int, int> quadFaces;

    // Multimap to group faces by vertex indices
    std::multimap<std::pair<float, float>, int> vertexMap;
    
    // Populate the vertexMap with face indices grouped by vertex x and y positions
    for (size_t i = 0; i < faces.size(); ++i) {
        const face& f = faces[i];
        vertexMap.insert({{vertices[f.a].x, vertices[f.a].y}, static_cast<int>(i)});
        vertexMap.insert({{vertices[f.b].x, vertices[f.b].y}, static_cast<int>(i)});
        vertexMap.insert({{vertices[f.c].x, vertices[f.c].y}, static_cast<int>(i)});
    }

    // Iterate through the vertexMap and find pairs of faces forming quads
    for (auto it = vertexMap.begin(); it != vertexMap.end(); ++it) {
        auto range = vertexMap.equal_range(it->first);
        std::set<int> faceSet;
        for (auto itr = range.first; itr != range.second; ++itr) {
            faceSet.insert(itr->second);
        }

        // Check for pairs of faces with two unique x and y positions
        for (auto itr1 = faceSet.begin(); itr1 != faceSet.end(); ++itr1) {
            for (auto itr2 = std::next(itr1); itr2 != faceSet.end(); ++itr2) {
                int faceIdx1 = *itr1;
                int faceIdx2 = *itr2;

                // Extract vertices of both faces
                const face& face1 = faces[faceIdx1];
                const face& face2 = faces[faceIdx2];

                std::set<std::tuple<float, float>> uniquePositions;
                uniquePositions.insert({vertices[face1.a].x, vertices[face1.a].y});
                uniquePositions.insert({vertices[face1.b].x, vertices[face1.b].y});
                uniquePositions.insert({vertices[face1.c].x, vertices[face1.c].y});
                uniquePositions.insert({vertices[face2.a].x, vertices[face2.a].y});
                uniquePositions.insert({vertices[face2.b].x, vertices[face2.b].y});
                uniquePositions.insert({vertices[face2.c].x, vertices[face2.c].y});

                if (uniquePositions.size() == 2) {
                    quadFaces[faceIdx1] = faceIdx2;
                }
            }
        }
    }

    return quadFaces;
}



vec3::vec3(int fx = 0, int fy = 0, int fz = 0) :x(fx), y(fy), z(fz) {}

mesh::mesh() {
}

mesh::~mesh() {
  if(mtype == meshtype::FLOOR) {
    g_meshFloors.erase(remove(g_meshFloors.begin(), g_meshFloors.end(), this), g_meshFloors.end());
  } else if(mtype == meshtype::V_WALL) {
    g_meshVWalls.erase(remove(g_meshVWalls.begin(), g_meshVWalls.end(), this), g_meshVWalls.end());
  } else if(mtype == meshtype::COLLISION) {
    g_meshCollisions.erase(remove(g_meshCollisions.begin(), g_meshCollisions.end(), this), g_meshCollisions.end());
  } else if(mtype == meshtype::OCCLUDER) {
    g_meshOccluders.erase(remove(g_meshOccluders.begin(), g_meshOccluders.end(), this), g_meshOccluders.end());
  } else if(mtype == meshtype::DECORATIVE) {

  }

  //add support for sharing textures later
  if(texture != nullptr) {
    SDL_DestroyTexture(texture);
  }
}

// Function to calculate the normal of a face
array<float, 3> calculateNormal(const vertex3d& v0, const vertex3d& v1, const vertex3d& v2) {
    array<float, 3> normal;
    float x1 = v1.x - v0.x;
    float y1 = v1.y - v0.y;
    float z1 = v1.z - v0.z;
    float x2 = v2.x - v0.x;
    float y2 = v2.y - v0.y;
    float z2 = v2.z - v0.z;

    normal[0] = y1 * z2 - z1 * y2;
    normal[1] = z1 * x2 - x1 * z2;
    normal[2] = x1 * y2 - y1 * x2;

    float length = sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
    normal[0] /= length;
    normal[1] /= length;
    normal[2] /= length;

    return normal;
}

// Function to calculate the vertex colors based on the light direction
void setVertexColors(vector<vertex3d>& vertices, const vector<face>& faces, const array<float, 3>& lightDir, meshtype mtype) {
    // Initialize vertex normals and counts
    for (auto& vertex : vertices) {
        vertex.normal = {0.0f, 0.0f, 0.0f};
    }

    // Accumulate normals for each vertex
    for (const auto& f : faces) {
        array<float, 3> normal = calculateNormal(vertices[f.a], vertices[f.b], vertices[f.c]);
        vertex3d* verticesArray[3] = {&vertices[f.a], &vertices[f.b], &vertices[f.c]};
        for (int i = 0; i < 3; ++i) {
            verticesArray[i]->normal[0] += normal[0];
            verticesArray[i]->normal[1] += normal[1];
            verticesArray[i]->normal[2] += normal[2];
        }
    }

    // Normalize and set vertex colors
    for (auto& vertex : vertices) {
        float length = sqrt(vertex.normal[0] * vertex.normal[0] + vertex.normal[1] * vertex.normal[1] + vertex.normal[2] * vertex.normal[2]);
        vertex.normal[0] /= length;
        vertex.normal[1] /= length;
        vertex.normal[2] /= length;

        float dotProduct = max(0.0f, vertex.normal[0] * lightDir[0] + vertex.normal[1] * lightDir[1] + vertex.normal[2] * lightDir[2]);
        //dotProduct = 0.2 + 0.8*dotProduct;
        Uint8 intensity = static_cast<Uint8>(255 * dotProduct);
        if(mtype == meshtype::V_WALL) {
          //don't change red channel
          vertex.color.g = intensity;
          vertex.color.b = intensity;
          vertex.color.a = 255;
        } else {
          vertex.color = {intensity, intensity, intensity, 255};
        }
    }
}


mesh* loadMeshFromPly(string faddress, vec3 forigin, float scale, meshtype fmtype) {
    string address = "resources/static/meshes/" + faddress + ".ply";
    vector<vertex3d> vertices;
    vector<face> faces;
    mesh* result = new mesh();
    result->origin = forigin;
    result->mtype = fmtype;

    if(fmtype == meshtype::FLOOR) {
        g_meshFloors.push_back(result);
    } else if (fmtype == meshtype::COLLISION) {
        g_meshCollisions.push_back(result);
    } else if(fmtype == meshtype::OCCLUDER) {
        g_meshOccluders.push_back(result);
    } else if(fmtype == meshtype::V_WALL) {
      g_meshVWalls.push_back(result);
    } else if(fmtype == meshtype::DECORATIVE) {
      g_meshDecorative.push_back(result); 
    }

    string binAddress = "";

    if (PHYSFS_exists(address.c_str())) {
        PHYSFS_ErrorCode error = PHYSFS_getLastErrorCode();
        PHYSFS_file* myfile = PHYSFS_openRead(address.c_str());
        error = PHYSFS_getLastErrorCode();

        if (error != 0) {
            cerr << "Error opening file: " << address << " Error: " << PHYSFS_getErrorByCode(error) << endl;
            abort();
        }

        PHYSFS_sint64 filesize = PHYSFS_fileLength(myfile);
        char* buf = new char[filesize];
        PHYSFS_readBytes(myfile, buf, filesize);
        PHYSFS_close(myfile);

        // Use happly to parse the buffer
        istringstream plyStream(string(buf, filesize));
        happly::PLYData plyIn(plyStream);

        delete[] buf;

        // Get vertex data
        vector<array<double, 3>> vertexData = plyIn.getVertexPositions();
        vector<array<unsigned char, 3>> vertexColors;
        vector<array<double, 2>> vertexUVs;
        vector<array<double, 2>> vertexLUVs;

        // Check if the .ply file contains color and UV data
        if (plyIn.hasElement("vertex") && plyIn.getElement("vertex").hasProperty("red")) {
            vertexColors = plyIn.getVertexColors();
        }

        if (plyIn.hasElement("vertex") && plyIn.getElement("vertex").hasProperty("s")) {
            vertexUVs = plyIn.getVertexUVs();
        }

        if (plyIn.hasElement("vertex") && plyIn.getElement("vertex").hasProperty("u")) {
            vertexLUVs = plyIn.getVertexLUVs();
        }

        // Convert to vertex3d
        for (size_t i = 0; i < vertexData.size(); ++i) {
            vertex3d v;
            v.x = static_cast<float>(vertexData[i][0]);
            v.y = static_cast<float>(vertexData[i][1]);
            v.z = static_cast<float>(vertexData[i][2]);

            if (i < vertexUVs.size()) {
                v.u = static_cast<float>(vertexUVs[i][0]);
                v.v = 1 - static_cast<float>(vertexUVs[i][1]);
            }
            if( i < vertexLUVs.size()) {
                v.lu = static_cast<float>(vertexLUVs[i][0]);
                v.lv = 1 - static_cast<float>(vertexLUVs[i][1]);
            }

            if( i < vertexColors.size()) {
              v.color.r = vertexColors[i][0];
              v.color.g = vertexColors[i][1];
              v.color.b = vertexColors[i][2];
            }

            vertices.push_back(v);
        }


        // Get face data
        vector<vector<size_t>> faceIndices = plyIn.getFaceIndices<size_t>();

        // Convert to face objects
        for (const auto& f : faceIndices) {
            if (f.size() == 3) { // Ensure it's a triangle
              faces.push_back({f[0], f[1], f[2]});
            } else {
                E("");
                E("Non triangular face in mesh");
                D(f.size());
                D(faces.size());
                E("");
            }
        }

        if(
            fmtype == meshtype::COLLISION ||
            fmtype == meshtype::FLOOR
            ) {
          const array<float, 3> lightDir = {0, -0.4472, -0.8944};
          setVertexColors(vertices, faces, lightDir, fmtype);
        } else if(fmtype == meshtype::V_WALL) {
          const array<float, 3> lightDir = {0, -0.707, -0.707};
          setVertexColors(vertices, faces, lightDir, fmtype);
        }
        

        // Transform 3D coordinates to 2D and set up SDL_Vertex array
        result->numVertices = faces.size() * 3;
        result->vertex = new SDL_Vertex[result->numVertices];
        result->vertexExtraData = vector<pair<float, float>>(result->numVertices);

        int index = 0;
        float maxDistanceFromOrigin = 0;
        for (const auto& f : faces) {
            auto convertVertex = [&](const vertex3d& v) {
                result->vertex[index].position.x = ((-v.x) * scale);
                result->vertex[index].position.y = ((v.y * scale)) * XtoY;
                float dist = Distance(result->vertex[index].position.x, result->vertex[index].position.y * XtoY, 0, 0);
                if(fmtype != meshtype::OCCLUDER || v.color.r < 128) {
                  result->vertex[index].position.y -= ((v.z * scale)) * XtoZ;
                  result->vertex[index].tex_coord.y = v.v;
                } else {
                  result->vertex[index].tex_coord.y = v.z * scale;
                  result->vertex[index].position.y = v.y * scale * XtoY;
                }
                result->vertex[index].color = v.color;
                result->vertex[index].tex_coord.x = v.u;
                result->vertexExtraData[index].first = v.lu;
                result->vertexExtraData[index].second = v.lv;
                result->vertex[index].color.r = v.color.r;
                result->vertex[index].color.g = v.color.g;
                result->vertex[index].color.b = v.color.b;

                if(dist > maxDistanceFromOrigin) {
                    maxDistanceFromOrigin = dist;
                }

                ++index;
            };

            convertVertex(vertices[f.a]);
            convertVertex(vertices[f.b]);
            convertVertex(vertices[f.c]);
        }

        result->sleepRadius = maxDistanceFromOrigin;
        result->faces = faces;
        for(auto& x : vertices) {
            x.x *= -scale;
            x.y *= scale * XtoY;
            x.z *= scale;
        }
        result->vertices = vertices;
    } else {
        cerr << "File does not exist: " << address << endl;
    }

    return result;
}
