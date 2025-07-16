#include <SDL3_image/SDL_image.h>

#include "L_asset.h"
#include "L_file.h"
#include "L_log.h"
#include "L_memory.h"
#include "L_mod.h"
#include "L_script.h"

static char asset_file_helper[FILE_PATH_MAX];

// Shaders
SOURCE_ASSET(shaders, shader, struct Shader*, ShaderID);

void load_shader(const char* name) {
    if (get_shader(name) != NULL)
        return;

    // Vertex shader
    SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "shaders/%s.vs", name);
    const char* file = get_mod_file(asset_file_helper, NULL);
    if (file == NULL) {
        WARN("Vertex shader for \"%s\" not found", name);
        return;
    }

    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);

    GLchar* code = SDL_LoadFile(file, NULL);
    if (code == NULL)
        FATAL("Shader \"%s\" vertex load fail: %s", name, SDL_GetError());
    glShaderSource(vertex, 1, (const GLchar* const*)&code, NULL);
    glCompileShader(vertex);

    GLint success;
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        lame_realloc(&code, 1024); // Why not reuse the code string?
        glGetShaderInfoLog(vertex, 1024, NULL, code);
        FATAL("Shader \"%s\" vertex fail:\n%s", name, code);
    }
    lame_free(&code);

    // Fragment shader
    SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "shaders/%s.fs", name);
    file = get_mod_file(asset_file_helper, NULL);
    if (file == NULL)
        FATAL("Fragment shader for \"%s\" not found", name);

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);

    code = (GLchar*)SDL_LoadFile(file, NULL);
    if (code == NULL)
        FATAL("Shader \"%s\" fragment load fail: %s", name, SDL_GetError());
    glShaderSource(fragment, 1, (const GLchar* const*)&code, NULL);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        lame_realloc(&code, 1024);
        glGetShaderInfoLog(fragment, 1024, NULL, code);
        FATAL("Shader \"%s\" fragment fail:\n%s", name, code);
    }

    // Shader struct
    struct Shader* shader = lame_alloc(sizeof(struct Shader));

    // General
    shader->hid = (ShaderID)create_handle(shader_handles, shader);
    shader->name = SDL_strdup(name);
    shader->transient = true; // No sense in unloading shaders

    // Program
    shader->program = glCreateProgram();
    glAttachShader(shader->program, vertex);
    glAttachShader(shader->program, fragment);
    glBindAttribLocation(shader->program, VATT_POSITION, "i_position");
    glBindAttribLocation(shader->program, VATT_NORMAL, "i_normal");
    glBindAttribLocation(shader->program, VATT_COLOR, "i_color");
    glBindAttribLocation(shader->program, VATT_UV, "i_uv");
    glBindAttribLocation(shader->program, VATT_BONE_INDEX, "i_bone_index");
    glBindAttribLocation(shader->program, VATT_BONE_WEIGHT, "i_bone_weight");
    glLinkProgram(shader->program);

    glGetProgramiv(shader->program, GL_LINK_STATUS, &success);
    if (!success) {
        lame_realloc(&code, 1024);
        glGetProgramInfoLog(shader->program, 1024, NULL, code);
        FATAL("Shader \"%s\" program fail:\n%s", name, code);
    }
    lame_free(&code);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    // Uniforms
    if ((shader->uniforms = SDL_CreateProperties()) == 0)
        FATAL("Shader \"%s\" uniforms fail: %s", name, SDL_GetError());
    GLsizei unum, ulen;
    glGetProgramiv(shader->program, GL_ACTIVE_UNIFORMS, &unum);
    glGetProgramiv(shader->program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &ulen);
    for (GLsizei i = 0; i < unum; i++) {
        GLchar uname[256];
        glGetActiveUniform(shader->program, i, ulen, NULL, NULL, NULL, uname);
        if (!SDL_SetNumberProperty(shader->uniforms, uname, (Sint64)glGetUniformLocation(shader->program, uname)))
            FATAL("Shader \"%s\" uniform \"%s\" fail: %s", name, uname, SDL_GetError());
    }

    shader->userdata = create_pointer_ref("shader", shader);
    ASSET_SANITY_PUSH(shader, shaders);
    INFO("Loaded shader \"%s\" (%u)", name, shader->hid);
}

void destroy_shader(struct Shader* shader) {
    ASSET_SANITY_POP(shader, shaders);
    unreference_pointer(&shader->userdata);

    glDeleteProgram(shader->program);
    SDL_DestroyProperties(shader->uniforms);

    INFO("Freed shader \"%s\" (%u)", shader->name, shader->hid);
    destroy_handle(shader_handles, shader->hid);
    lame_free(&shader->name);
    lame_free(&shader);
}

// Textures
SOURCE_ASSET(textures, texture, struct Texture*, TextureID);

void load_texture(const char* name) {
    if (get_texture(name) != NULL)
        return;

    SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "textures/%s.*", name);
    const char* file = get_mod_file(asset_file_helper, ".json");
    if (file == NULL) {
        WARN("Texture \"%s\" not found", name);
        return;
    }

    SDL_Surface* surface = IMG_Load(file);
    if (surface == NULL) {
        WTF("Texture \"%s\" image fail: %s", SDL_GetError());
        return;
    }

    // Texture struct
    struct Texture* texture = lame_alloc(sizeof(struct Texture));

    // General
    texture->hid = (TextureID)create_handle(texture_handles, texture);
    texture->name = SDL_strdup(name);
    texture->transient = false;

    // Inheritance
    texture->parent = NULL;
    texture->children = NULL;
    texture->num_children = 0;

    // Data
    texture->size[0] = surface->w;
    texture->size[1] = surface->h;
    texture->offset[0] = texture->offset[1] = 0;
    texture->uvs[0] = texture->uvs[1] = 0;
    texture->uvs[2] = texture->uvs[3] = 1;

    glGenTextures(1, &texture->texture);
    glBindTexture(GL_TEXTURE_2D, texture->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0);

    GLint format;
    switch (surface->format) {
        default: {
            SDL_Surface* temp = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
            if (temp == NULL)
                FATAL("Texture \"%s\" image conversion fail: %s", SDL_GetError());
            SDL_DestroySurface(surface);
            surface = temp;

            format = GL_RGBA8;
            break;
        }

        case SDL_PIXELFORMAT_RGB24:
            format = GL_RGB8;
            break;
        case SDL_PIXELFORMAT_RGB48:
            format = GL_RGB16;
            break;
        case SDL_PIXELFORMAT_RGB48_FLOAT:
            format = GL_RGB16F;
            break;
        case SDL_PIXELFORMAT_RGB96_FLOAT:
            format = GL_RGB32F;
            break;

        case SDL_PIXELFORMAT_RGBA32:
            format = GL_RGBA8;
            break;
        case SDL_PIXELFORMAT_RGBA64:
            format = GL_RGBA16;
            break;
        case SDL_PIXELFORMAT_RGBA64_FLOAT:
            format = GL_RGBA16F;
            break;
        case SDL_PIXELFORMAT_RGBA128_FLOAT:
            format = GL_RGBA32F;
            break;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
    SDL_DestroySurface(surface);

    texture->userdata = create_pointer_ref("texture", texture);
    ASSET_SANITY_PUSH(texture, textures);
    INFO("Loaded texture \"%s\" (%u)", name, texture->hid);
}

void destroy_texture(struct Texture* texture) {
    ASSET_SANITY_POP(texture, textures);
    unreference_pointer(&texture->userdata);

    if (texture->children != NULL) {
        for (int i = 0; i < texture->num_children; i++)
            destroy_texture_hid(texture->children[i]);
        lame_free(&texture->children);
    }
    if (texture->parent == NULL)
        glDeleteTextures(1, &texture->texture);

    INFO("Freed texture \"%s\" (%u)", texture->name, texture->hid);
    destroy_handle(texture_handles, texture->hid);
    lame_free(&texture->name);
    lame_free(&texture);
}

// Materials
SOURCE_ASSET(materials, material, struct Material*, MaterialID);

void load_material(const char* name) {
    if (get_material(name) != NULL)
        return;

    SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "materials/%s.*", name);
    const char* file = get_mod_file(asset_file_helper, NULL);
    if (file == NULL)
        WARN("Material \"%s\" not found, making a placeholder", name);

    // Material struct
    struct Material* material = lame_alloc(sizeof(struct Material));

    // General
    material->hid = (MaterialID)create_handle(material_handles, material);
    material->name = SDL_strdup(name);
    material->transient = false;

    // Textures
    material->textures[0] = material->textures[1] = NULL;
    material->num_textures[0] = material->num_textures[1] = 0;
    glm_vec2_zero(material->texture_speed);

    // Properties
    material->filter = true;
    glm_vec4_one(material->color);
    material->alpha_test = 0.5;
    material->bright = 0;
    glm_vec2_zero(material->scroll);
    material->specular[0] = 0;
    material->specular[1] = 1;
    material->rimlight[0] = 0;
    material->rimlight[1] = 1;
    material->half_lambert = false;
    material->cel = 0;
    glm_vec3_zero(material->wind);

    if (file != NULL) {
        yyjson_doc* json = load_json(file);
        if (json != NULL) {
            yyjson_val* root = yyjson_doc_get_root(json);
            if (yyjson_is_obj(root)) {
                yyjson_val* value = yyjson_obj_get(root, "texture");
                if (yyjson_is_str(value)) {
                    TextureID* textures = lame_alloc(sizeof(TextureID));
                    textures[0] = fetch_texture_hid(yyjson_get_str(value));
                    material->textures[0] = textures;
                    material->num_textures[0] = 1;
                }

                if (yyjson_is_str(value = yyjson_obj_get(root, "blend_texture"))) {
                    TextureID* textures = lame_alloc(sizeof(TextureID));
                    textures[0] = fetch_texture_hid(yyjson_get_str(value));
                    material->textures[1] = textures;
                    material->num_textures[1] = 1;
                }

                if (yyjson_is_real(value = yyjson_obj_get(root, "texture_speed")))
                    material->texture_speed[0] = yyjson_get_real(value);
                if (yyjson_is_real(value = yyjson_obj_get(root, "blend_texture_speed")))
                    material->texture_speed[1] = yyjson_get_real(value);
                if (yyjson_is_bool(value = yyjson_obj_get(root, "filter")))
                    material->filter = yyjson_get_bool(value);

                if (yyjson_is_arr(value = yyjson_obj_get(root, "color")) && yyjson_arr_size(value) >= 4) {
                    material->color[0] = yyjson_get_real(yyjson_arr_get(value, 0));
                    material->color[1] = yyjson_get_real(yyjson_arr_get(value, 1));
                    material->color[2] = yyjson_get_real(yyjson_arr_get(value, 2));
                    material->color[3] = yyjson_get_real(yyjson_arr_get(value, 3));
                }

                if (yyjson_is_real(value = yyjson_obj_get(root, "alpha_test")))
                    material->alpha_test = yyjson_get_real(value);
                if (yyjson_is_real(value = yyjson_obj_get(root, "bright")))
                    material->bright = yyjson_get_real(value);

                if (yyjson_is_arr(value = yyjson_obj_get(root, "scroll")) && yyjson_arr_size(value) >= 2) {
                    material->scroll[0] = yyjson_get_real(yyjson_arr_get(value, 0));
                    material->scroll[1] = yyjson_get_real(yyjson_arr_get(value, 1));
                }

                if (yyjson_is_arr(value = yyjson_obj_get(root, "specular")) && yyjson_arr_size(value) >= 2) {
                    material->specular[0] = yyjson_get_real(yyjson_arr_get(value, 0));
                    material->specular[1] = yyjson_get_real(yyjson_arr_get(value, 1));
                }

                if (yyjson_is_arr(value = yyjson_obj_get(root, "rimlight")) && yyjson_arr_size(value) >= 2) {
                    material->rimlight[0] = yyjson_get_real(yyjson_arr_get(value, 0));
                    material->rimlight[1] = yyjson_get_real(yyjson_arr_get(value, 1));
                }

                if (yyjson_is_bool(value = yyjson_obj_get(root, "half_lambert")))
                    material->half_lambert = yyjson_get_bool(value);
                if (yyjson_is_real(value = yyjson_obj_get(root, "cel")))
                    material->cel = yyjson_get_real(value);

                if (yyjson_is_arr(value = yyjson_obj_get(root, "wind")) && yyjson_arr_size(value) >= 3) {
                    material->wind[0] = yyjson_get_real(yyjson_arr_get(value, 0));
                    material->wind[1] = yyjson_get_real(yyjson_arr_get(value, 1));
                    material->wind[2] = yyjson_get_real(yyjson_arr_get(value, 2));
                }
            } else {
                WTF("Expected material \"%s\" root as object, got %s", name, yyjson_get_type_desc(root));
            }

            yyjson_doc_free(json);
        }
    }

    material->userdata = create_pointer_ref("material", material);
    ASSET_SANITY_PUSH(material, materials);
    INFO("Loaded material \"%s\" (%u)", name, material->hid);
}

void destroy_material(struct Material* material) {
    ASSET_SANITY_POP(material, materials);
    unreference_pointer(&material->userdata);

    FREE_POINTER(material->textures[0]);
    FREE_POINTER(material->textures[1]);

    INFO("Freed material \"%s\" (%u)", material->name, material->hid);
    destroy_handle(material_handles, material->hid);
    lame_free(&material->name);
    lame_free(&material);
}

// Models
SOURCE_ASSET(models, model, struct Model*, ModelID);

static struct Node* read_node(uint8_t** cursor, struct Node* parent) {
    struct Node* node = lame_alloc(sizeof(struct Node));
    node->name = read_string(cursor);
    node->index = read_f32(cursor);
    node->bone = read_bool(cursor);

    node->dq[0] = read_u32(cursor);
    node->dq[1] = read_u32(cursor);
    node->dq[2] = read_u32(cursor);
    node->dq[3] = read_u32(cursor);
    node->dq[4] = read_u32(cursor);
    node->dq[5] = read_u32(cursor);
    node->dq[6] = read_u32(cursor);
    node->dq[7] = read_u32(cursor);

    for (size_t i = read_u32(cursor); i > 0; i--) // Mesh count
        read_u32(cursor);                         // Mesh index

    node->parent = parent;
    if ((node->num_children = read_u32(cursor)) > 0) {
        node->children = lame_alloc(node->num_children * sizeof(struct Node*));
        for (size_t i = 0; i < node->num_children; i++)
            node->children[i] = read_node(cursor, node);
    } else {
        node->children = NULL;
    }

    return node;
}

static void destroy_node(struct Node* node) {
    if (node->children != NULL) {
        for (size_t i = 0; i < node->num_children; i++)
            destroy_node(node->children[i]);
        lame_free(&(node->children));
    }

    lame_free(&(node->name));
    lame_free(&node);
}

void load_model(const char* name) {
    if (get_model(name) != NULL)
        return;

    SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "models/%s.bbmod", name);
    const char* file = get_mod_file(asset_file_helper, NULL);
    if (file == NULL) {
        WARN("Model \"%s\" not found", name);
        return;
    }

    void* buffer = SDL_LoadFile(file, NULL);
    if (buffer == NULL) {
        WTF("Model \"%s\" load fail: %s", name, SDL_GetError());
        return;
    }
    uint8_t* cursor = (uint8_t*)buffer;

    // File header
    bool has_minor;
    if (SDL_strcmp((const char*)cursor, "bbmod") == 0) {
        has_minor = false;
        cursor += sizeof("bbmod");
    } else if (SDL_strcmp((const char*)cursor, "BBMOD") == 0) {
        has_minor = true;
        cursor += sizeof("BBMOD");
    } else {
        FATAL("Invalid header in model \"%s\"");
    }

    uint8_t major = read_u8(&cursor);
    uint8_t minor = 0;
    if (major != BBMOD_VERSION_MAJOR)
        FATAL("Bad BBMOD major version in model \"%s\" (%u =/= %u)", name, major, BBMOD_VERSION_MAJOR);
    if (has_minor) {
        uint8_t minor = read_u8(&cursor);
        if (minor < 2)
            FATAL(
                "Bad BBMOD version in model \"%s\" (%u.%u < %u.%u)", name, major, minor, BBMOD_VERSION_MAJOR,
                BBMOD_VERSION_MINOR
            );
        if (minor > BBMOD_VERSION_MINOR)
            FATAL(
                "Bad BBMOD version in model \"%s\" (%u.%u > %u.%u)", name, major, minor, BBMOD_VERSION_MAJOR,
                BBMOD_VERSION_MINOR
            );
    }

    // Model struct
    struct Model* model = lame_alloc(sizeof(struct Model));

    // General
    model->hid = (ModelID)create_handle(model_handles, model);
    model->name = SDL_strdup(name);
    model->transient = false;

    // Submodels
    if ((model->num_submodels = read_u32(&cursor)) > 0) {
        model->submodels = lame_alloc(model->num_submodels * sizeof(struct Submodel));
        for (size_t i = 0; i < model->num_submodels; i++) {
            struct Submodel* submodel = &(model->submodels[i]);

            // Material index
            submodel->material = read_u32(&cursor);

            // Bounding box
            read_f32(&cursor);
            read_f32(&cursor);
            read_f32(&cursor);
            read_f32(&cursor);
            read_f32(&cursor);
            read_f32(&cursor);

            /* Vertex format
               lameo only needs the following attributes:
               - Position
               - Normals
               - UVs
               - Secondary UVs
               - Colors
               - Bone Indices
               - Bone Weights

               The rest are bogus:
                - Tangents
                - Indices */
            bool has_position = read_bool(&cursor);
            bool has_normals = read_bool(&cursor);
            bool has_uvs = read_bool(&cursor);
            bool has_uvs2 = read_bool(&cursor);
            bool has_color = read_bool(&cursor);
            bool has_tangents = read_bool(&cursor);
            bool has_bones = read_bool(&cursor);
            bool has_id = read_bool(&cursor);
            read_u32(&cursor); // Skip primitive type (it's always GL_TRIANGLES)

            // Vertices
            submodel->num_vertices = read_u32(&cursor);
            submodel->vertices = lame_alloc(submodel->num_vertices * sizeof(struct WorldVertex));

            for (size_t j = 0; j < submodel->num_vertices; j++) {
                struct WorldVertex* vertex = &(submodel->vertices[j]);

                if (has_position) {
                    vertex->position[0] = read_f32(&cursor);
                    vertex->position[1] = read_f32(&cursor);
                    vertex->position[2] = read_f32(&cursor);
                } else {
                    glm_vec3_zero(vertex->position);
                }

                if (has_normals) {
                    vertex->normal[0] = read_f32(&cursor);
                    vertex->normal[1] = read_f32(&cursor);
                    vertex->normal[2] = read_f32(&cursor);
                } else {
                    vertex->normal[0] = vertex->normal[1] = 0;
                    vertex->normal[2] = -1;
                }

                if (has_uvs) {
                    vertex->uv[0] = read_f32(&cursor);
                    vertex->uv[1] = read_f32(&cursor);
                } else {
                    vertex->uv[0] = vertex->uv[1] = 0;
                }

                if (has_uvs2) {
                    vertex->uv[2] = read_f32(&cursor);
                    vertex->uv[3] = read_f32(&cursor);
                } else {
                    vertex->uv[2] = vertex->uv[3] = 0;
                }

                if (has_color) {
                    vertex->color[0] = read_u8(&cursor);
                    vertex->color[1] = read_u8(&cursor);
                    vertex->color[2] = read_u8(&cursor);
                    vertex->color[3] = read_u8(&cursor);
                } else {
                    vertex->color[0] = vertex->color[1] = vertex->color[2] = vertex->color[3] = 255;
                }

                if (has_tangents) {
                    read_f32(&cursor);
                    read_f32(&cursor);
                    read_f32(&cursor);
                    read_f32(&cursor);
                }

                if (has_bones) {
                    vertex->bone_index[0] = read_f32(&cursor);
                    vertex->bone_index[1] = read_f32(&cursor);
                    vertex->bone_index[2] = read_f32(&cursor);
                    vertex->bone_index[3] = read_f32(&cursor);

                    vertex->bone_weight[0] = read_f32(&cursor);
                    vertex->bone_weight[1] = read_f32(&cursor);
                    vertex->bone_weight[2] = read_f32(&cursor);
                    vertex->bone_weight[3] = read_f32(&cursor);
                } else {
                    vertex->bone_index[0] = vertex->bone_index[1] = vertex->bone_index[2] = vertex->bone_index[3] = 0;
                    glm_vec4_zero(vertex->bone_weight);
                }
            }

            // VAO and VBO
            glGenVertexArrays(1, &submodel->vao);
            glBindVertexArray(submodel->vao);
            glEnableVertexArrayAttrib(submodel->vao, VATT_POSITION);
            glVertexArrayAttribFormat(submodel->vao, VATT_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3);
            glEnableVertexArrayAttrib(submodel->vao, VATT_NORMAL);
            glVertexArrayAttribFormat(submodel->vao, VATT_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3);
            glEnableVertexArrayAttrib(submodel->vao, VATT_COLOR);
            glVertexArrayAttribFormat(submodel->vao, VATT_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GLubyte) * 4);
            glEnableVertexArrayAttrib(submodel->vao, VATT_UV);
            glVertexArrayAttribFormat(submodel->vao, VATT_UV, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4);
            glEnableVertexArrayAttrib(submodel->vao, VATT_BONE_INDEX);
            glVertexArrayAttribFormat(
                submodel->vao, VATT_BONE_INDEX, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(GLubyte) * 4
            );
            glEnableVertexArrayAttrib(submodel->vao, VATT_BONE_WEIGHT);
            glVertexArrayAttribFormat(submodel->vao, VATT_BONE_WEIGHT, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4);

            glGenBuffers(1, &submodel->vbo);
            glBindBuffer(GL_ARRAY_BUFFER, submodel->vbo);
            glBufferData(
                GL_ARRAY_BUFFER, sizeof(struct WorldVertex) * submodel->num_vertices, submodel->vertices, GL_STATIC_DRAW
            );

            glEnableVertexAttribArray(VATT_POSITION);
            glVertexAttribPointer(
                VATT_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(struct WorldVertex),
                (void*)offsetof(struct WorldVertex, position)
            );

            glEnableVertexAttribArray(VATT_NORMAL);
            glVertexAttribPointer(
                VATT_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(struct WorldVertex),
                (void*)offsetof(struct WorldVertex, normal)
            );

            glEnableVertexAttribArray(VATT_COLOR);
            glVertexAttribPointer(
                VATT_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(struct WorldVertex),
                (void*)offsetof(struct WorldVertex, color)
            );

            glEnableVertexAttribArray(VATT_UV);
            glVertexAttribPointer(
                VATT_UV, 4, GL_FLOAT, GL_FALSE, sizeof(struct WorldVertex), (void*)offsetof(struct WorldVertex, uv)
            );

            glEnableVertexAttribArray(VATT_BONE_INDEX);
            glVertexAttribPointer(
                VATT_BONE_INDEX, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(struct WorldVertex),
                (void*)offsetof(struct WorldVertex, bone_index)
            );

            glEnableVertexAttribArray(VATT_BONE_WEIGHT);
            glVertexAttribPointer(
                VATT_BONE_WEIGHT, 4, GL_FLOAT, GL_FALSE, sizeof(struct WorldVertex),
                (void*)offsetof(struct WorldVertex, bone_weight)
            );
        }
    } else {
        model->submodels = NULL;
    }

    // Nodes
    model->root_node = ((model->num_nodes = read_u32(&cursor)) > 0) ? read_node(&cursor, NULL) : NULL;

    // Bones
    if ((model->num_bones = read_u32(&cursor)) > 0) {
        model->bone_offsets = lame_alloc(model->num_bones * sizeof(DualQuaternion));
        for (size_t i = 0; i < model->num_bones; i++) {
            DualQuaternion* dq = &(model->bone_offsets[(size_t)read_f32(&cursor)]);
            (*dq)[0] = read_f32(&cursor);
            (*dq)[1] = read_f32(&cursor);
            (*dq)[2] = read_f32(&cursor);
            (*dq)[3] = read_f32(&cursor);
            (*dq)[4] = read_f32(&cursor);
            (*dq)[5] = read_f32(&cursor);
            (*dq)[6] = read_f32(&cursor);
            (*dq)[7] = read_f32(&cursor);
        }
    } else {
        model->bone_offsets = NULL;
    }

    // Materials
    if ((model->num_materials = read_u32(&cursor)) > 0) {
        model->materials = lame_alloc(model->num_materials * sizeof(MaterialID));
        for (size_t i = 0; i < model->num_materials; i++) {
            const char* material_name = read_string(&cursor);
            model->materials[i] = fetch_material_hid(material_name);
            lame_free(&material_name);
        }
    } else {
        model->materials = NULL;
    }

    lame_free(&buffer);

    model->userdata = create_pointer_ref("model", model);
    ASSET_SANITY_PUSH(model, models);
    INFO("Loaded model \"%s\" (%u)", name, model->hid);
}

void destroy_model(struct Model* model) {
    ASSET_SANITY_POP(model, models);
    unreference_pointer(&model->userdata);

    if (model->submodels != NULL) {
        for (size_t i = 0; i < model->num_submodels; i++) {
            struct Submodel* submodel = &(model->submodels[i]);
            glDeleteVertexArrays(1, &submodel->vao);
            glDeleteBuffers(1, &submodel->vbo);
            lame_free(&submodel->vertices);
        }
        lame_free(&model->submodels);
    }

    CLOSE_POINTER(model->root_node, destroy_node);
    FREE_POINTER(model->bone_offsets);
    FREE_POINTER(model->materials);

    INFO("Freed model \"%s\" (%u)", model->name, model->hid);
    destroy_handle(model_handles, model->hid);
    lame_free(&model->name);
    lame_free(&model);
}

// Fonts
SOURCE_ASSET(fonts, font, struct Font*, FontID);

void load_font(const char* name) {
    if (get_font(name) != NULL)
        return;

    SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "fonts/%s.json", name);
    const char* file = get_mod_file(asset_file_helper, NULL);
    if (file == NULL) {
        WARN("Font \"%s\" not found", name);
        return;
    }

    yyjson_doc* json = load_json(file);
    if (json == NULL) {
        WTF("Font \"%s\" load fail", name);
        return;
    }

    yyjson_val* root = yyjson_doc_get_root(json);
    if (!yyjson_is_obj(root)) {
        WTF("Expected root object in \"%s.json\", got %s", name, yyjson_get_type_desc(root));
        yyjson_doc_free(json);
        return;
    }

    struct Font* font = lame_alloc(sizeof(struct Font));

    // General
    font->hid = (FontID)create_handle(font_handles, font);
    font->name = SDL_strdup(name);
    font->transient = false;

    // Data
    font->texture = 0;
    font->size = 0;
    font->glyphs = NULL;
    font->num_glyphs = 0;

    // Texture
    struct Texture* texture;
    yyjson_val* value = yyjson_obj_get(root, "texture");
    if (yyjson_is_str(value)) {
        if ((texture = fetch_texture(yyjson_get_str(value))) == NULL)
            FATAL("Font texture \"%s\" not found", name);
        font->texture = texture->hid;
    } else {
        FATAL("Expected \"texture\" as string in \"%s.json\", got %s", name, yyjson_get_type_desc(value));
    }

    // Glyphs
    value = yyjson_obj_get(root, "size");
    if (!yyjson_is_uint(value))
        FATAL("Expected \"size\" as uint in \"%s.json\", got %s", name, yyjson_get_type_desc(value));
    if ((font->size = yyjson_get_uint(value)) <= 0)
        FATAL("Expected non-zero size for font \"%s\"", name);

    value = yyjson_obj_get(root, "glyphs");
    if (!yyjson_is_obj(value))
        FATAL("Expected \"glyphs\" as object in \"%s.json\", got %s", name, yyjson_get_type_desc(value));

    size_t gldef = 0;
    size_t i, n;
    yyjson_val *key, *val;
    yyjson_obj_foreach(value, i, n, key, val) {
        if (!yyjson_is_obj(val))
            FATAL(
                "Expected glyph \"%c\" as object in \"%s.json\", got %s", yyjson_get_str(key)[0], name,
                yyjson_get_type_desc(value)
            );

        const char* character = yyjson_get_str(key);
        size_t gid = SDL_StepUTF8(&character, NULL);
        if (font->num_glyphs <= gid) {
            if (font->glyphs == NULL) {
                font->num_glyphs = gid + 1;
                font->glyphs = lame_alloc(font->num_glyphs * sizeof(struct Glyph*));
                lame_set(font->glyphs, 0, font->num_glyphs * sizeof(struct Glyph*));
            } else {
                size_t old_num = font->num_glyphs;
                font->num_glyphs = gid + 1;
                lame_realloc(&font->glyphs, font->num_glyphs * sizeof(struct Glyph*));
                lame_set(font->glyphs + old_num, 0, (font->num_glyphs - old_num) * sizeof(struct Glyph*));
            }
        }

        struct Glyph* glyph = font->glyphs[gid];
        if (glyph == NULL) {
            glyph = font->glyphs[gid] = lame_alloc(sizeof(struct Glyph));
            ++gldef;
        } else
            WARN("Font \"%s\" overwriting glyph \"%c\"", name, gid);
        glyph->size[0] = (GLfloat)yyjson_get_uint(yyjson_obj_get(val, "width"));
        glyph->size[1] = (GLfloat)yyjson_get_uint(yyjson_obj_get(val, "height"));
        glyph->offset[0] = (GLfloat)yyjson_get_num(yyjson_obj_get(val, "x_offset"));
        glyph->offset[1] = (GLfloat)yyjson_get_num(yyjson_obj_get(val, "y_offset"));
        glyph->uvs[0] = ((GLfloat)yyjson_get_uint(yyjson_obj_get(val, "x"))) / texture->size[0];
        glyph->uvs[1] = ((GLfloat)yyjson_get_uint(yyjson_obj_get(val, "y"))) / texture->size[1];
        glyph->uvs[2] = glyph->uvs[0] + (glyph->size[0] / texture->size[0]);
        glyph->uvs[3] = glyph->uvs[1] + (glyph->size[1] / texture->size[1]);
        glyph->advance = (GLfloat)yyjson_get_num(yyjson_obj_get(val, "advance"));
    }

    yyjson_doc_free(json);

    font->userdata = create_pointer_ref("font", font);
    ASSET_SANITY_PUSH(font, fonts);
    INFO("Loaded font \"%s\" (%u, %u/%u glyphs)", name, font->hid, gldef, font->num_glyphs);
}

void destroy_font(struct Font* font) {
    ASSET_SANITY_POP(font, fonts);
    unreference_pointer(&font->userdata);

    if (font->glyphs != NULL) {
        for (size_t i = 0; i < font->num_glyphs; i++)
            FREE_POINTER(font->glyphs[i]);
        lame_free(&font->glyphs);
    }

    INFO("Freed font \"%s\" (%u)", font->name, font->hid);
    destroy_handle(font_handles, font->hid);
    lame_free(&font->name);
    lame_free(&font);
}

// Sounds
SOURCE_ASSET(sounds, sound, struct Sound*, SoundID);

struct Sound* create_sound(const char* name) {
    struct Sound* sound = lame_alloc(sizeof(struct Sound));

    // General
    sound->hid = (SoundID)create_handle(sound_handles, sound);
    sound->name = SDL_strdup(name);
    sound->transient = false;

    // Data
    sound->samples = NULL;
    sound->num_samples = 0;
    sound->gain = 1;
    sound->pitch[0] = sound->pitch[1] = 1;

    sound->userdata = create_pointer_ref("sound", sound);

    return sound;
}

void load_sound(const char* name) {
    if (get_sound(name) != NULL)
        return;

    struct Sound* sound = NULL;

    // Find a JSON for definitions
    SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "sounds/%s.json", name);
    const char* file = get_mod_file(asset_file_helper, NULL);
    if (file == NULL) {
        // Fall back to just using a sample
        SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "sounds/%s.*", name);
        if ((file = get_mod_file(asset_file_helper, ".json")) == NULL) {
            WARN("Sound \"%s\" not found", name);
            return;
        }

        sound = create_sound(name);

        // Data
        sound->samples = lame_alloc(sizeof(struct Sample*));
        sound->num_samples = 1;
        load_sample(file, &sound->samples[0]);

        goto sound_loaded;
    }

    yyjson_doc* json = load_json(file);
    if (json == NULL) {
        WTF("Sound \"%s\" load fail", name);
        return;
    }

    yyjson_val* root = yyjson_doc_get_root(json);
    if (!yyjson_is_obj(root)) {
        WTF("Expected root object in \"%s.json\", got %s", name, yyjson_get_type_desc(root));
        yyjson_doc_free(json);
        return;
    }

    // Allocate empty sound at this point
    sound = create_sound(name);

    // Samples
    yyjson_val* value = yyjson_obj_get(root, "sample");
    if (yyjson_is_str(value)) {
        sound->samples = lame_alloc(sizeof(struct Sample*));
        sound->num_samples = 1;

        SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "sounds/%s.*", yyjson_get_str(value));
        if ((file = get_mod_file(asset_file_helper, ".json")) == NULL) {
            sound->samples[0] = NULL;
            WARN("Sample \"%s\" not found", asset_file_helper);
        } else {
            load_sample(file, &sound->samples[0]);
        }
    } else if (yyjson_is_arr(value)) {
        sound->num_samples = yyjson_arr_size(value);
        sound->samples = lame_alloc(sound->num_samples * sizeof(struct Sample*));

        size_t i, n;
        yyjson_val* entry;
        yyjson_arr_foreach(value, i, n, entry) {
            if (yyjson_is_null(entry)) {
                sound->samples[i] = NULL;
                continue;
            } else if (!yyjson_is_str(entry)) {
                sound->samples[i] = NULL;
                WTF("Expected \"sample\" index %u as string or null, got %s", i, yyjson_get_type_desc(entry));
                continue;
            }

            SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "sounds/%s.*", yyjson_get_str(entry));
            if ((file = get_mod_file(asset_file_helper, ".json")) == NULL) {
                sound->samples[i] = NULL;
                WARN("Sample \"%s\" not found", asset_file_helper);
                continue;
            }
            load_sample(file, &sound->samples[i]);
        }
    } else if (!yyjson_is_null(value)) {
        WTF("Expected \"sample\" as string, array or null in \"%s.json\", got %s", name, yyjson_get_type_desc(value));
    }

    // Gain
    if (yyjson_is_num(value = yyjson_obj_get(root, "gain")))
        sound->gain = (float)yyjson_get_num(value);

    // Pitch
    value = yyjson_obj_get(root, "pitch");
    if (yyjson_is_arr(value)) {
        if (yyjson_arr_size(value) >= 2) {
            sound->pitch[0] = yyjson_get_num(yyjson_arr_get(value, 0));
            sound->pitch[1] = yyjson_get_num(yyjson_arr_get(value, 1));
        }
    } else if (yyjson_is_num(value)) {
        sound->pitch[0] = sound->pitch[1] = yyjson_get_num(value);
    }

    yyjson_doc_free(json);

sound_loaded:
    ASSET_SANITY_PUSH(sound, sounds);
    INFO("Loaded sound \"%s\" (%u)", name, sound->hid);
}

void destroy_sound(struct Sound* sound) {
    ASSET_SANITY_POP(sound, sounds);
    unreference_pointer(&sound->userdata);

    if (sound->samples != NULL) {
        for (size_t i = 0; i < sound->num_samples; i++)
            if (sound->samples[i] != NULL)
                destroy_sample(sound->samples[i]);
        lame_free(&sound->samples);
    }

    INFO("Freed sound \"%s\" (%u)", sound->name, sound->hid);
    destroy_handle(sound_handles, sound->hid);
    lame_free(&sound->name);
    lame_free(&sound);
}

// Music
SOURCE_ASSET(music, track, struct Track*, TrackID);

void load_track(const char* name) {
    if (get_track(name) != NULL)
        return;

    SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "music/%s.*", name);
    const char* track_file = get_mod_file(asset_file_helper, ".json");
    if (track_file == NULL) {
        WARN("Track \"%s\" not found", name);
        return;
    }

    struct Track* track = lame_alloc(sizeof(struct Track));

    // General
    track->hid = (TrackID)create_handle(track_handles, track);
    track->name = SDL_strdup(name);
    track->transient = false;

    // Data
    load_stream(track_file, &track->stream);

    track->userdata = create_pointer_ref("track", track);
    ASSET_SANITY_PUSH(track, music);
    INFO("Loaded track \"%s\" (%u)", name, track->hid);
}

void destroy_track(struct Track* track) {
    ASSET_SANITY_POP(track, music);
    unreference_pointer(&track->userdata);

    destroy_stream(track->stream);
    INFO("Freed track \"%s\" (%u)", track->name, track->hid);
    destroy_handle(track_handles, track->hid);
    lame_free(&track->name);
    lame_free(&track);
}

void asset_init() {
    shaders_init();
    textures_init();
    materials_init();
    models_init();
    fonts_init();
    sounds_init();
    music_init();

    INFO("Opened");
}

void asset_teardown() {
    clear_assets(true);

    shaders_teardown();
    textures_teardown();
    materials_teardown();
    models_teardown();
    fonts_teardown();
    sounds_teardown();
    music_teardown();

    INFO("Closed");
}

void clear_assets(bool teardown) {
    clear_shaders(teardown);
    clear_textures(teardown);
    clear_materials(teardown);
    clear_models(teardown);
    clear_fonts(teardown);
    clear_sounds(teardown);
    clear_music(teardown);
}
