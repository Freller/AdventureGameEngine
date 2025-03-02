#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "happly.h"

#include "mesh.h"
void checkAndSetEdgeInfo(edgeInfo& ei, mesh* m) {
    struct VertexComparator {
        bool operator()(const SDL_Vertex& lhs, const SDL_Vertex& rhs) const {
            return std::tie(lhs.position.x, lhs.position.y) < std::tie(rhs.position.x, rhs.position.y);
        }
    };

    const float tolerance = 3.0f;

    auto addVertexToSet = [&](std::set<SDL_Vertex, VertexComparator>& vertexSet, const SDL_Vertex& vertex) {
        for (const auto& v : vertexSet) {
            if (std::fabs(v.position.x - vertex.position.x) < tolerance &&
                std::fabs(v.position.y - vertex.position.y) < tolerance) {
                return;
            }
        }
        vertexSet.insert(vertex);
    };

    for (const auto& f : m->faces) {
        SDL_Vertex va = m->vertex[f.a];
        SDL_Vertex vb = m->vertex[f.b];
        SDL_Vertex vc = m->vertex[f.c];
        SDL_Vertex vd = m->vertex[f.d];

        va.position.x += m->origin.x;
        va.position.y += m->origin.y;

        vb.position.x += m->origin.x;
        vb.position.y += m->origin.y;

        vc.position.x += m->origin.x;
        vc.position.y += m->origin.y;

        vd.position.x += m->origin.x;
        vd.position.y += m->origin.y;

        std::set<SDL_Vertex, VertexComparator> vertexSet;
        addVertexToSet(vertexSet, va);
        addVertexToSet(vertexSet, vb);
        addVertexToSet(vertexSet, vc);
        addVertexToSet(vertexSet, vd);
        addVertexToSet(vertexSet, ei.first);
        addVertexToSet(vertexSet, ei.second);

        if (vertexSet.size() <= 4) {
            ei.wallMesh = m;
            ei.indices = {f.a, f.b, f.c, f.d};
            return; // Exit the loop once a match is found
        }
    }
}

int main() {
    // Example usage
    mesh m;
    // Initialize m.faces and m.vertex with sample data...

    edgeInfo ei;
    // Initialize ei.first and ei.second with edge vertices...

    checkAndSetEdgeInfo(ei, &m);

    if (ei.wallMesh != nullptr) {
        std::cout << "Match found! Indices: ";
        for (int index : ei.indices) {
            std::cout << index << " ";
        }
        std::cout << std::endl;
    } else {
        std::cout << "No match found." << std::endl;
    }

    return 0;
}vec3::vec3(int fx = 0, int fy = 0, int fz = 0) :x(fx), y(fy), z(fz) {}

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
      if(f.d < 100) {
        array<float, 3> normal = calculateNormal(vertices[f.a], vertices[f.b], vertices[f.c]);
        vertex3d* verticesArray[4] = {&vertices[f.a], &vertices[f.b], &vertices[f.c], &vertices[f.d]};
        for (int i = 0; i < 4; ++i) {
          verticesArray[i]->normal[0] += normal[0];
          verticesArray[i]->normal[1] += normal[1];
          verticesArray[i]->normal[2] += normal[2];
        }
      } else {
        array<float, 3> normal = calculateNormal(vertices[f.a], vertices[f.b], vertices[f.c]);
        vertex3d* verticesArray[3] = {&vertices[f.a], &vertices[f.b], &vertices[f.c]};
        for (int i = 0; i < 3; ++i) {
          verticesArray[i]->normal[0] += normal[0];
          verticesArray[i]->normal[1] += normal[1];
          verticesArray[i]->normal[2] += normal[2];
        }
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
        vector<vector<size_t>> faceIndices;
        if(fmtype != meshtype::OCCLUDER) { //occluders have edges and no faces
          faceIndices = plyIn.getFaceIndices<size_t>();
        }

        // Convert to face objects
        for (const auto& f : faceIndices) {
            if (f.size() == 4) { // Ensure it's a triangle
              faces.push_back({f[0], f[1], f[2], f[3]});
            } else if (f.size() == 3 && fmtype == meshtype::FLOOR) {
              face n;
              n.a = f[0];
              n.b = f[1];
              n.c = f[2];
              n.d = 100;
              faces.push_back(n);
            } else {
                E("");
                E("Floors can have quads or tris, walls and collisions must have quads, and occluders can have edges.");
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
//          const array<float, 3> lightDir = {0, -0.707, -0.707};
//          setVertexColors(vertices, faces, lightDir, fmtype);
        }

        

        // Transform 3D coordinates to 2D and set up SDL_Vertex array
        result->vertex = new SDL_Vertex[vertices.size()];
        result->vertexExtraData = vector<pair<float, float>>(vertices.size());

        for (const auto& f : faces) {
            int fail = 0;

            if( fmtype == meshtype::V_WALL) {
              if(vertices[f.a].color.r < 128 && vertices[f.b].color.r < 128) {
                vertex3d first = vertices[f.a];
                vertex3d second = vertices[f.b];
                SDL_Vertex A;
                
                A.position.x = ((-first.x) * scale);
                A.position.y = ((first.y * scale)) * XtoY - ((first.z * scale)) * XtoZ;
    
                SDL_Vertex B;
    
                B.position.x = ((-second.x) * scale);
                B.position.y = ((second.y * scale)) * XtoY - ((second.z * scale)) * XtoZ;

                A.position.x += forigin.x;
                A.position.y += forigin.y;
                B.position.x += forigin.x;
                B.position.y += forigin.y;

                edgeInfo ei;
                ei.first = A;
                ei.second = B;
                g_wEdges.emplace_back(ei);
                fail++;
              }

              if(vertices[f.a].color.r < 128 && vertices[f.c].color.r < 128) {
                if(fail) {E("Bad V_WALL, make sure only the bottom verts have 0 red"); abort();}
                vertex3d first = vertices[f.a];
                vertex3d second = vertices[f.c];
                SDL_Vertex A;
                
                A.position.x = ((-first.x) * scale);
                A.position.y = ((first.y * scale)) * XtoY - ((first.z * scale)) * XtoZ;
    
                SDL_Vertex B;
    
                B.position.x = ((-second.x) * scale);
                B.position.y = ((second.y * scale)) * XtoY - ((second.z * scale)) * XtoZ;

                A.position.x += forigin.x;
                A.position.y += forigin.y;
                B.position.x += forigin.x;
                B.position.y += forigin.y;

                edgeInfo ei;
                ei.first = A;
                ei.second = B;
                g_wEdges.emplace_back(ei);
                fail++;
              }

              if(vertices[f.c].color.r < 128 && vertices[f.b].color.r < 128) {
                if(fail) {E("Bad V_WALL, make sure only the bottom verts have 0 red"); abort();}
                vertex3d first = vertices[f.c];
                vertex3d second = vertices[f.b];
                SDL_Vertex A;
                
                A.position.x = ((-first.x) * scale);
                A.position.y = ((first.y * scale)) * XtoY - ((first.z * scale)) * XtoZ;
    
                SDL_Vertex B;
    
                B.position.x = ((-second.x) * scale);
                B.position.y = ((second.y * scale)) * XtoY - ((second.z * scale)) * XtoZ;

                A.position.x += forigin.x;
                A.position.y += forigin.y;
                B.position.x += forigin.x;
                B.position.y += forigin.y;

                edgeInfo ei;
                ei.first = A;
                ei.second = B;
                g_wEdges.emplace_back(ei);
              }
            }

        }
       


        int index = 0;
        float maxDistanceFromOrigin = 0;
        for( auto v : vertices) {
            result->vertex[index].position.x = ((-v.x) * scale);
            result->vertex[index].position.y = ((v.y * scale)) * XtoY;
            float dist = Distance(result->vertex[index].position.x, result->vertex[index].position.y * XtoY, 0, 0);
            result->vertex[index].position.y -= ((v.z * scale)) * XtoZ;
            result->vertex[index].tex_coord.y = v.v;
            result->vertex[index].color = v.color;
            result->vertex[index].tex_coord.x = v.u;
            result->vertexExtraData[index].first = v.lu;
            result->vertexExtraData[index].second = v.lv;
//            result->vertex[index].color.r = v.color.r;
//            result->vertex[index].color.g = v.color.g;
//            result->vertex[index].color.b = v.color.b;
            result->vertex[index].color.r = 255;
            result->vertex[index].color.g = 255;
            result->vertex[index].color.b = 255;


            if(dist > maxDistanceFromOrigin) {
              maxDistanceFromOrigin = dist;
            }

            ++index;
        }

        result->numVertices = vertices.size();
        
        result->indices = new int[faces.size() * 6];
        result->numIndices = 0;
        for (const auto& f : faces) {
          if(f.d < 100) {
            result->indices[result->numIndices] = f.a;
            result->numIndices++;
            result->indices[result->numIndices] = f.b;
            result->numIndices++;
            result->indices[result->numIndices] = f.c;
            result->numIndices++;
  
            result->indices[result->numIndices] = f.a;
            result->numIndices++;
            result->indices[result->numIndices] = f.c;
            result->numIndices++;
            result->indices[result->numIndices] = f.d;
            result->numIndices++;
          } else {
            result->indices[result->numIndices] = f.a;
            result->numIndices++;
            result->indices[result->numIndices] = f.b;
            result->numIndices++;
            result->indices[result->numIndices] = f.c;
            result->numIndices++;
          }
        }

        if(fmtype == meshtype::OCCLUDER) {
          vector<array<int, 2>> edgeData = plyIn.getEdges();
          for(array<int,2> n : edgeData) {
            vertex3d first = vertices[n[0]];
            vertex3d second = vertices[n[1]];

            SDL_Vertex A;
            
            A.position.x = ((-first.x) * scale);
            A.position.y = ((first.y * scale)) * XtoY - ((first.z * scale)) * XtoZ;
            A.position.x += forigin.x;
            A.position.y += forigin.y;
            A.color.r = 0;
            A.color.g = 0;
            A.color.b = 0;
            A.color.a = 255;

            SDL_Vertex B;

            B.position.x = ((-second.x) * scale);
            B.position.y = ((second.y * scale)) * XtoY - ((second.z * scale)) * XtoZ;

            B.position.x += forigin.x;
            B.position.y += forigin.y;
            B.color.r = 0;
            B.color.g = 0;
            B.color.b = 0;
            B.color.a = 255;

            edgeInfo ei;
            ei.first = A;
            ei.second = B;
 
            checkAndSetEdgeInfo(ei, g_meshVWalls[0]);

            g_oEdges.emplace_back(ei);
          }
        }



        result->sleepRadius = maxDistanceFromOrigin;
        result->faces = faces;
        for(auto& x : vertices) {
            x.x *= -scale;
            x.y *= scale * XtoY;
            x.z *= scale;
        }

        //needed for collisions and floors
        if(fmtype == meshtype::COLLISION ||
            fmtype == meshtype::FLOOR ||
            fmtype == meshtype::V_WALL) {
          result->vertices = vertices;
        }
    } else {
        cerr << "File does not exist: " << address << endl;
    }

    return result;
}
