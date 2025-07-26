#include <SDL3_image/SDL_image.h>

#include "L_asset.h"
#include "L_file.h"
#include "L_log.h"
#include "L_memory.h"
#include "L_mod.h"
#include "L_script.h"
#include "L_tick.h"

static char asset_file_helper[FILE_PATH_MAX];

// Shaders
SOURCE_ASSET(shaders, shader, struct Shader*);

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
    struct Shader* shader = lame_alloc_clean(sizeof(struct Shader));

    // General
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
    shader->uniforms = SDL_CreateProperties();
    if (shader->uniforms == 0)
        FATAL("Shader \"%s\" uniforms fail: %s", name, SDL_GetError());
    GLsizei unum;
    glGetProgramiv(shader->program, GL_ACTIVE_UNIFORMS, &unum);
    for (GLsizei i = 0; i < unum; i++) {
        GLchar uname[256];
        glGetActiveUniform(shader->program, i, 256, NULL, NULL, NULL, uname);
        if (!SDL_SetNumberProperty(shader->uniforms, uname, (Sint64)glGetUniformLocation(shader->program, uname)))
            FATAL("Shader \"%s\" uniform \"%s\" fail: %s", name, uname, SDL_GetError());
    }

    shader->userdata = create_pointer_ref("shader", shader);
    ASSET_SANITY_PUSH(shader, shaders);
    DEBUG("Loaded shader \"%s\" (%u)", name, shader);
}

void destroy_shader(struct Shader* shader) {
    ASSET_SANITY_POP(shader, shaders);
    unreference_pointer(&(shader->userdata));

    glDeleteProgram(shader->program);
    SDL_DestroyProperties(shader->uniforms);

    DEBUG("Freed shader \"%s\" (%u)", shader->name, shader);
    lame_free(&(shader->name));
    lame_free(&shader);
}

// Textures
SOURCE_ASSET(textures, texture, struct Texture*);

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
        WTF("Texture \"%s\" image fail: %s", name, SDL_GetError());
        return;
    }

    // Texture struct
    struct Texture* texture = lame_alloc_clean(sizeof(struct Texture));

    // General
    texture->name = SDL_strdup(name);

    // Data
    texture->size[0] = surface->w;
    texture->size[1] = surface->h;
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
    DEBUG("Loaded texture \"%s\" (%u)", name, texture);
}

void destroy_texture(struct Texture* texture) {
    ASSET_SANITY_POP(texture, textures);
    unreference_pointer(&(texture->userdata));

    if (texture->parent == NULL)
        glDeleteTextures(1, &(texture->texture));

    DEBUG("Freed texture \"%s\" (%u)", texture->name, texture);
    lame_free(&(texture->name));
    lame_free(&texture);
}

// Materials
SOURCE_ASSET(materials, material, struct Material*);

void load_material(const char* name) {
    if (get_material(name) != NULL)
        return;

    SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "materials/%s.*", name);
    const char* file = get_mod_file(asset_file_helper, NULL);
    if (file == NULL)
        WARN("Material \"%s\" not found, making a placeholder", name);

    // Material struct
    struct Material* material = lame_alloc_clean(sizeof(struct Material));

    // General
    material->name = SDL_strdup(name);

    // Properties
    material->filter = true;
    glm_vec4_one(material->color);
    material->alpha_test = 0.5f;
    material->specular[1] = 1;
    material->rimlight[1] = 1;

    if (file != NULL) {
        yyjson_doc* json = load_json(file);
        if (json != NULL) {
            yyjson_val* root = yyjson_doc_get_root(json);
            if (yyjson_is_obj(root)) {
                yyjson_val* value = yyjson_obj_get(root, "texture");
                if (yyjson_is_str(value)) {
                    struct Texture** textures = (struct Texture**)lame_alloc(sizeof(struct Texture*));
                    textures[0] = fetch_texture(yyjson_get_str(value));
                    material->textures[0] = textures;
                    material->num_textures[0] = 1;
                }

                value = yyjson_obj_get(root, "blend_texture");
                if (yyjson_is_str(value)) {
                    struct Texture** textures = (struct Texture**)lame_alloc(sizeof(struct Texture*));
                    textures[1] = fetch_texture(yyjson_get_str(value));
                    material->textures[1] = textures;
                    material->num_textures[1] = 1;
                }

                value = yyjson_obj_get(root, "texture_speed");
                if (yyjson_is_real(value))
                    material->texture_speed[0] = (float)yyjson_get_real(value);
                value = yyjson_obj_get(root, "blend_texture_speed");
                if (yyjson_is_real(value))
                    material->texture_speed[1] = (float)yyjson_get_real(value);
                value = yyjson_obj_get(root, "filter");
                if (yyjson_is_bool(value))
                    material->filter = yyjson_get_bool(value);

                value = yyjson_obj_get(root, "color");
                if (yyjson_is_arr(value) && yyjson_arr_size(value) >= 4) {
                    material->color[0] = (float)yyjson_get_real(yyjson_arr_get(value, 0));
                    material->color[1] = (float)yyjson_get_real(yyjson_arr_get(value, 1));
                    material->color[2] = (float)yyjson_get_real(yyjson_arr_get(value, 2));
                    material->color[3] = (float)yyjson_get_real(yyjson_arr_get(value, 3));
                }

                value = yyjson_obj_get(root, "alpha_test");
                if (yyjson_is_real(value))
                    material->alpha_test = (float)yyjson_get_real(value);
                value = yyjson_obj_get(root, "bright");
                if (yyjson_is_real(value))
                    material->bright = (float)yyjson_get_real(value);

                value = yyjson_obj_get(root, "scroll");
                if (yyjson_is_arr(value) && yyjson_arr_size(value) >= 2) {
                    material->scroll[0] = (float)yyjson_get_real(yyjson_arr_get(value, 0));
                    material->scroll[1] = (float)yyjson_get_real(yyjson_arr_get(value, 1));
                }

                value = yyjson_obj_get(root, "specular");
                if (yyjson_is_arr(value) && yyjson_arr_size(value) >= 2) {
                    material->specular[0] = (float)yyjson_get_real(yyjson_arr_get(value, 0));
                    material->specular[1] = (float)yyjson_get_real(yyjson_arr_get(value, 1));
                }

                value = yyjson_obj_get(root, "rimlight");
                if (yyjson_is_arr(value) && yyjson_arr_size(value) >= 2) {
                    material->rimlight[0] = (float)yyjson_get_real(yyjson_arr_get(value, 0));
                    material->rimlight[1] = (float)yyjson_get_real(yyjson_arr_get(value, 1));
                }

                value = yyjson_obj_get(root, "half_lambert");
                if (yyjson_is_bool(value))
                    material->half_lambert = yyjson_get_bool(value);
                value = yyjson_obj_get(root, "cel");
                if (yyjson_is_real(value))
                    material->cel = (float)yyjson_get_real(value);

                value = yyjson_obj_get(root, "wind");
                if (yyjson_is_arr(value) && yyjson_arr_size(value) >= 3) {
                    material->wind[0] = (float)yyjson_get_real(yyjson_arr_get(value, 0));
                    material->wind[1] = (float)yyjson_get_real(yyjson_arr_get(value, 1));
                    material->wind[2] = (float)yyjson_get_real(yyjson_arr_get(value, 2));
                }
            } else {
                WTF("Expected material \"%s\" root as object, got %s", name, yyjson_get_type_desc(root));
            }

            yyjson_doc_free(json);
        }
    }

    material->userdata = create_pointer_ref("material", material);
    ASSET_SANITY_PUSH(material, materials);
    DEBUG("Loaded material \"%s\" (%u)", name, material);
}

void destroy_material(struct Material* material) {
    ASSET_SANITY_POP(material, materials);
    unreference_pointer(&(material->userdata));

    FREE_POINTER(material->textures[0]);
    FREE_POINTER(material->textures[1]);

    DEBUG("Freed material \"%s\" (%u)", material->name, material);
    lame_free(&(material->name));
    lame_free(&material);
}

// Models
SOURCE_ASSET(models, model, struct Model*);

static struct Node* read_node(uint8_t** cursor, struct Node* parent) {
    struct Node* node = lame_alloc_clean(sizeof(struct Node));
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
    node->num_children = read_u32(cursor);
    if ((node->num_children) > 0) {
        node->children = (struct Node**)lame_alloc(node->num_children * sizeof(struct Node*));
        for (size_t i = 0; i < node->num_children; i++)
            node->children[i] = read_node(cursor, node);
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
    uint8_t* cursor = buffer;

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
    struct Model* model = lame_alloc_clean(sizeof(struct Model));

    // General
    model->name = SDL_strdup(name);

    // Submodels
    model->num_submodels = read_u32(&cursor);
    if (model->num_submodels > 0) {
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
            submodel->vertices = lame_alloc_clean(submodel->num_vertices * sizeof(struct WorldVertex));

            for (size_t j = 0; j < submodel->num_vertices; j++) {
                struct WorldVertex* vertex = &(submodel->vertices[j]);

                if (has_position) {
                    vertex->position[0] = read_f32(&cursor);
                    vertex->position[1] = read_f32(&cursor);
                    vertex->position[2] = read_f32(&cursor);
                }

                if (has_normals) {
                    vertex->normal[0] = read_f32(&cursor);
                    vertex->normal[1] = read_f32(&cursor);
                    vertex->normal[2] = read_f32(&cursor);
                } else {
                    vertex->normal[2] = -1;
                }

                if (has_uvs) {
                    vertex->uv[0] = read_f32(&cursor);
                    vertex->uv[1] = read_f32(&cursor);
                }

                if (has_uvs2) {
                    vertex->uv[2] = read_f32(&cursor);
                    vertex->uv[3] = read_f32(&cursor);
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
                }

                if (has_id) {
                    read_f32(&cursor);
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
            glVertexArrayAttribFormat(submodel->vao, VATT_BONE_INDEX, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4);
            glEnableVertexArrayAttrib(submodel->vao, VATT_BONE_WEIGHT);
            glVertexArrayAttribFormat(submodel->vao, VATT_BONE_WEIGHT, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4);

            glGenBuffers(1, &submodel->vbo);
            glBindBuffer(GL_ARRAY_BUFFER, submodel->vbo);
            glBufferData(
                GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(struct WorldVertex) * submodel->num_vertices), submodel->vertices,
                GL_STATIC_DRAW
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
                VATT_BONE_INDEX, 4, GL_FLOAT, GL_FALSE, sizeof(struct WorldVertex),
                (void*)offsetof(struct WorldVertex, bone_index)
            );

            glEnableVertexAttribArray(VATT_BONE_WEIGHT);
            glVertexAttribPointer(
                VATT_BONE_WEIGHT, 4, GL_FLOAT, GL_FALSE, sizeof(struct WorldVertex),
                (void*)offsetof(struct WorldVertex, bone_weight)
            );
        }
    }

    // Nodes
    model->root_node = ((model->num_nodes = read_u32(&cursor)) > 0) ? read_node(&cursor, NULL) : NULL;

    // Bones
    model->num_bones = read_u32(&cursor);
    if (model->num_bones > 0) {
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
    }

    // Materials
    model->num_materials = read_u32(&cursor);
    if (model->num_materials > 0) {
        model->materials = (struct Material**)lame_alloc_clean(model->num_materials * sizeof(struct Material*));
        for (size_t i = 0; i < model->num_materials; i++) {
            const char* material_name = read_string(&cursor);
            if (material_name[0] != '\0')
                model->materials[i] = fetch_material(material_name);
            lame_free(&material_name);
        }
    }

    lame_free(&buffer);

    model->userdata = create_pointer_ref("model", model);
    ASSET_SANITY_PUSH(model, models);
    DEBUG("Loaded model \"%s\" (%u)", name, model);
}

void destroy_model(struct Model* model) {
    ASSET_SANITY_POP(model, models);
    unreference_pointer(&(model->userdata));

    if (model->submodels != NULL) {
        for (size_t i = 0; i < model->num_submodels; i++) {
            struct Submodel* submodel = &(model->submodels[i]);
            glDeleteVertexArrays(1, &(submodel->vao));
            glDeleteBuffers(1, &(submodel->vbo));
            lame_free(&(submodel->vertices));
        }
        lame_free(&(model->submodels));
    }

    CLOSE_POINTER(model->root_node, destroy_node);
    FREE_POINTER(model->bone_offsets);
    FREE_POINTER(model->materials);

    DEBUG("Freed model \"%s\" (%u)", model->name, model);
    lame_free(&(model->name));
    lame_free(&model);
}

// Animations
SOURCE_ASSET(animations, animation, struct Animation*);

void load_animation(const char* name) {
    if (get_animation(name) != NULL)
        return;

    SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "animations/%s.bbanim", name);
    const char* file = get_mod_file(asset_file_helper, NULL);
    if (file == NULL) {
        WARN("Animation \"%s\" not found", name);
        return;
    }

    void* buffer = SDL_LoadFile(file, NULL);
    if (buffer == NULL) {
        WTF("Animation \"%s\" load fail: %s", SDL_GetError());
        return;
    }
    uint8_t* cursor = buffer;

    // File header
    bool has_minor;
    if (SDL_strcmp((const char*)cursor, "bbanim") == 0) {
        has_minor = false;
        cursor += sizeof("bbanim");
    } else if (SDL_strcmp((const char*)cursor, "BBANIM") == 0) {
        has_minor = true;
        cursor += sizeof("BBANIM");
    } else {
        FATAL("Invalid header in animation \"%s\"");
    }

    uint8_t major = read_u8(&cursor);
    uint8_t minor = 0;
    if (major != BBMOD_VERSION_MAJOR)
        FATAL("Bad BBMOD major version in animation \"%s\" (%u =/= %u)", name, major, BBMOD_VERSION_MAJOR);
    if (has_minor) {
        uint8_t minor = read_u8(&cursor);
        if (minor < 2)
            FATAL(
                "Bad BBMOD version in animation \"%s\" (%u.%u < %u.%u)", name, major, minor, BBMOD_VERSION_MAJOR,
                BBMOD_VERSION_MINOR
            );
        if (minor > BBMOD_VERSION_MINOR)
            FATAL(
                "Bad BBMOD version in animation \"%s\" (%u.%u > %u.%u)", name, major, minor, BBMOD_VERSION_MAJOR,
                BBMOD_VERSION_MINOR
            );
    }

    // Animation struct
    struct Animation* animation = lame_alloc_clean(sizeof(struct Animation));

    // General
    animation->name = SDL_strdup(name);

    // Data
    enum BoneSpaces spaces = (enum BoneSpaces)(read_u8(&cursor));
    animation->num_frames = (size_t)(read_f64(&cursor));
    animation->frame_speed = (float)(read_f64(&cursor) / (double)TICKRATE);

    animation->num_nodes = (size_t)(read_u32(&cursor));
    animation->num_bones = (size_t)(read_u32(&cursor));
    if (spaces & BS_PARENT)
        animation->parent_frames = (DualQuaternion**)lame_alloc(animation->num_frames * sizeof(DualQuaternion*));
    if (spaces & BS_WORLD)
        animation->world_frames = (DualQuaternion**)lame_alloc(animation->num_frames * sizeof(DualQuaternion*));
    if (spaces & BS_BONE)
        animation->bone_frames = (DualQuaternion**)lame_alloc(animation->num_frames * sizeof(DualQuaternion*));
    ;

    for (size_t i = 0; i < animation->num_frames; i++) {
        if (animation->parent_frames != NULL) {
            DualQuaternion* frame = (DualQuaternion*)lame_alloc(animation->num_nodes * sizeof(DualQuaternion));
            for (size_t j = 0; j < animation->num_nodes; j++) {
                frame[j][0] = read_f32(&cursor);
                frame[j][1] = read_f32(&cursor);
                frame[j][2] = read_f32(&cursor);
                frame[j][3] = read_f32(&cursor);
                frame[j][4] = read_f32(&cursor);
                frame[j][5] = read_f32(&cursor);
                frame[j][6] = read_f32(&cursor);
                frame[j][7] = read_f32(&cursor);
            }
            animation->parent_frames[i] = frame;
        }

        if (animation->world_frames != NULL) {
            DualQuaternion* frame = lame_alloc(animation->num_nodes * sizeof(DualQuaternion));
            for (size_t j = 0; j < animation->num_nodes; j++) {
                frame[j][0] = read_f32(&cursor);
                frame[j][1] = read_f32(&cursor);
                frame[j][2] = read_f32(&cursor);
                frame[j][3] = read_f32(&cursor);
                frame[j][4] = read_f32(&cursor);
                frame[j][5] = read_f32(&cursor);
                frame[j][6] = read_f32(&cursor);
                frame[j][7] = read_f32(&cursor);
            }
            animation->world_frames[i] = frame;
        }

        if (animation->bone_frames != NULL) {
            DualQuaternion* frame = lame_alloc(animation->num_bones * sizeof(DualQuaternion));
            for (size_t j = 0; j < animation->num_bones; j++) {
                frame[j][0] = read_f32(&cursor);
                frame[j][1] = read_f32(&cursor);
                frame[j][2] = read_f32(&cursor);
                frame[j][3] = read_f32(&cursor);
                frame[j][4] = read_f32(&cursor);
                frame[j][5] = read_f32(&cursor);
                frame[j][6] = read_f32(&cursor);
                frame[j][7] = read_f32(&cursor);
            }
            animation->bone_frames[i] = frame;
        }
    }

    lame_free(&buffer);

    animation->userdata = create_pointer_ref("animation", animation);
    ASSET_SANITY_PUSH(animation, animations);
    DEBUG("Loaded animation \"%s\" (%u)", name, animation);
}

void destroy_animation(struct Animation* animation) {
    ASSET_SANITY_POP(animation, animations);
    unreference_pointer(&(animation->userdata));

    if (animation->parent_frames != NULL) {
        for (size_t i = 0; i < animation->num_frames; i++)
            lame_free(&(animation->parent_frames[i]));
        lame_free(&(animation->parent_frames));
    }
    if (animation->world_frames != NULL) {
        for (size_t i = 0; i < animation->num_frames; i++)
            lame_free(&(animation->world_frames[i]));
        lame_free(&(animation->world_frames));
    }
    if (animation->bone_frames != NULL) {
        for (size_t i = 0; i < animation->num_frames; i++)
            lame_free(&(animation->bone_frames[i]));
        lame_free(&(animation->bone_frames));
    }

    DEBUG("Freed animation \"%s\" (%u)", animation->name, animation);
    lame_free(&(animation->name));
    lame_free(&animation);
}

// Fonts
SOURCE_ASSET(fonts, font, struct Font*);

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

    struct Font* font = lame_alloc_clean(sizeof(struct Font));

    // General
    font->name = SDL_strdup(name);

    // Texture
    struct Texture* texture;
    yyjson_val* value = yyjson_obj_get(root, "texture");
    if (yyjson_is_str(value)) {
        texture = fetch_texture(yyjson_get_str(value));
        if (texture == NULL)
            FATAL("Font texture \"%s\" not found", name);
        font->texture = texture;
    } else {
        FATAL("Expected \"texture\" as string in \"%s.json\", got %s", name, yyjson_get_type_desc(value));
    }

    // Glyphs
    value = yyjson_obj_get(root, "size");
    if (!yyjson_is_uint(value))
        FATAL("Expected \"size\" as uint in \"%s.json\", got %s", name, yyjson_get_type_desc(value));
    font->size = (float)yyjson_get_uint(value);
    if (font->size <= 0)
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
                font->glyphs = (struct Glyph**)lame_alloc_clean(font->num_glyphs * sizeof(struct Glyph*));
            } else {
                const size_t old_num = font->num_glyphs;
                font->num_glyphs = gid + 1;
                lame_realloc_clean(
                    &(font->glyphs), old_num * sizeof(struct Glyph*), font->num_glyphs * sizeof(struct Glyph*)
                );
            }
        }

        struct Glyph* glyph = font->glyphs[gid];
        if (glyph == NULL) {
            glyph = font->glyphs[gid] = lame_alloc_clean(sizeof(struct Glyph));
            ++gldef;
        } else
            WARN("Font \"%s\" overwriting glyph \"%c\"", name, gid);
        glyph->size[0] = (GLfloat)yyjson_get_uint(yyjson_obj_get(val, "width"));
        glyph->size[1] = (GLfloat)yyjson_get_uint(yyjson_obj_get(val, "height"));
        glyph->offset[0] = (GLfloat)yyjson_get_num(yyjson_obj_get(val, "x_offset"));
        glyph->offset[1] = (GLfloat)yyjson_get_num(yyjson_obj_get(val, "y_offset"));
        glyph->uvs[0] = ((GLfloat)yyjson_get_uint(yyjson_obj_get(val, "x"))) / (GLfloat)texture->size[0];
        glyph->uvs[1] = ((GLfloat)yyjson_get_uint(yyjson_obj_get(val, "y"))) / (GLfloat)texture->size[1];
        glyph->uvs[2] = glyph->uvs[0] + (glyph->size[0] / (GLfloat)texture->size[0]);
        glyph->uvs[3] = glyph->uvs[1] + (glyph->size[1] / (GLfloat)texture->size[1]);
        glyph->advance = (GLfloat)yyjson_get_num(yyjson_obj_get(val, "advance"));
    }

    yyjson_doc_free(json);

    font->userdata = create_pointer_ref("font", font);
    ASSET_SANITY_PUSH(font, fonts);
    DEBUG("Loaded font \"%s\" (%u, %u/%u glyphs)", name, font, gldef, font->num_glyphs);
}

void destroy_font(struct Font* font) {
    ASSET_SANITY_POP(font, fonts);
    unreference_pointer(&(font->userdata));

    if (font->glyphs != NULL) {
        for (size_t i = 0; i < font->num_glyphs; i++)
            FREE_POINTER(font->glyphs[i]);
        lame_free(&(font->glyphs));
    }

    DEBUG("Freed font \"%s\" (%u)", font->name, font);
    lame_free(&(font->name));
    lame_free(&font);
}

// Sounds
SOURCE_ASSET(sounds, sound, struct Sound*);

struct Sound* create_sound(const char* name) {
    struct Sound* sound = lame_alloc_clean(sizeof(struct Sound));

    // General
    sound->name = SDL_strdup(name);

    // Data
    sound->gain = 1;
    glm_vec2_one(sound->pitch);

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
        file = get_mod_file(asset_file_helper, ".json");
        if (file == NULL) {
            WARN("Sound \"%s\" not found", name);
            return;
        }

        sound = create_sound(name);

        // Data
        sound->samples = (Sample**)lame_alloc(sizeof(Sample*));
        sound->num_samples = 1;
        load_sample(file, &(sound->samples[0]));

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
        sound->samples = (Sample**)lame_alloc(sizeof(Sample*));
        sound->num_samples = 1;

        SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "sounds/%s.*", yyjson_get_str(value));
        file = get_mod_file(asset_file_helper, ".json");
        if (file == NULL) {
            sound->samples[0] = NULL;
            WARN("Sample \"%s\" not found", asset_file_helper);
        } else {
            load_sample(file, &sound->samples[0]);
        }
    } else if (yyjson_is_arr(value)) {
        sound->num_samples = yyjson_arr_size(value);
        sound->samples = (Sample**)lame_alloc_clean(sound->num_samples * sizeof(Sample*));

        size_t i, n;
        yyjson_val* entry;
        yyjson_arr_foreach(value, i, n, entry) {
            if (yyjson_is_null(entry)) {
                continue;
            } else if (!yyjson_is_str(entry)) {
                WTF("Expected \"sample\" index %u as string or null, got %s", i, yyjson_get_type_desc(entry));
                continue;
            }

            SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "sounds/%s.*", yyjson_get_str(entry));
            file = get_mod_file(asset_file_helper, ".json");
            if (file == NULL) {
                WARN("Sample \"%s\" not found", asset_file_helper);
                continue;
            }
            load_sample(file, &(sound->samples[i]));
        }
    } else if (!yyjson_is_null(value)) {
        WTF("Expected \"sample\" as string, array or null in \"%s.json\", got %s", name, yyjson_get_type_desc(value));
    }

    // Gain
    value = yyjson_obj_get(root, "gain");
    if (yyjson_is_num(value))
        sound->gain = (float)yyjson_get_num(value);

    // Pitch
    value = yyjson_obj_get(root, "pitch");
    if (yyjson_is_arr(value)) {
        if (yyjson_arr_size(value) >= 2) {
            sound->pitch[0] = (float)yyjson_get_num(yyjson_arr_get(value, 0));
            sound->pitch[1] = (float)yyjson_get_num(yyjson_arr_get(value, 1));
        }
    } else if (yyjson_is_num(value)) {
        sound->pitch[0] = sound->pitch[1] = (float)yyjson_get_num(value);
    }

    yyjson_doc_free(json);

sound_loaded:
    ASSET_SANITY_PUSH(sound, sounds);
    DEBUG("Loaded sound \"%s\" (%u)", name, sound);
}

void destroy_sound(struct Sound* sound) {
    ASSET_SANITY_POP(sound, sounds);
    unreference_pointer(&(sound->userdata));

    if (sound->samples != NULL) {
        for (size_t i = 0; i < sound->num_samples; i++)
            if (sound->samples[i] != NULL)
                destroy_sample(sound->samples[i]);
        lame_free(&(sound->samples));
    }

    DEBUG("Freed sound \"%s\" (%u)", sound->name, sound);
    lame_free(&(sound->name));
    lame_free(&sound);
}

// Music
SOURCE_ASSET(music, track, struct Track*);

void load_track(const char* name) {
    if (get_track(name) != NULL)
        return;

    SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "music/%s.*", name);
    const char* track_file = get_mod_file(asset_file_helper, ".json");
    if (track_file == NULL) {
        WARN("Track \"%s\" not found", name);
        return;
    }

    struct Track* track = lame_alloc_clean(sizeof(struct Track));

    // General
    track->name = SDL_strdup(name);

    // Data
    load_stream(track_file, &track->stream);

    track->userdata = create_pointer_ref("track", track);
    ASSET_SANITY_PUSH(track, music);
    DEBUG("Loaded track \"%s\" (%u)", name, track);
}

void destroy_track(struct Track* track) {
    ASSET_SANITY_POP(track, music);
    unreference_pointer(&(track->userdata));

    destroy_stream(track->stream);
    DEBUG("Freed track \"%s\" (%u)", track->name, track);
    lame_free(&(track->name));
    lame_free(&track);
}

void asset_init() {
    shaders_init();
    textures_init();
    materials_init();
    models_init();
    animations_init();
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
    animations_teardown();
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
    clear_animations(teardown);
    clear_fonts(teardown);
    clear_sounds(teardown);
    clear_music(teardown);
}
