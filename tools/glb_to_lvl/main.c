#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <float.h>
#include <math.h>
#include "lvl.h"
#include "mini_math.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#define MODEL_SCALE 1.0f
#define MAX_CAMERAS 5
#define MAX_BOUNDS 1
#define MAX_COLLIDERS 10
#define MAX_LIGHTS 7
#define MAX_ZONES 10
#define MAX_JSON 1
#define MAX_OBJECTS 50
#define MAX_EXTRA_JSON_TOKENS 128

static int is_big_endian(){
	int i=1;
	return !*((char *)&i);
}

static uint8_t bswap_u8(uint8_t val){
	return val;
}

static uint16_t bswap_u16(uint16_t val){
	return (val << 8) | ((val >> 8));
}

static uint32_t bswap_u32(uint32_t val){
	val = ((val << 8) & 0xFF00FF00) | (( val >> 8) & 0xFF00FF);
	return (val << 16) | (val >> 16);
}
static uint32_t bswap_u64(uint64_t val){
	val = ((val << 8) & 0xFF00FF00FF00FF00ULL ) | ((val >> 8) & 0x00FF00FF00FF00FFULL );
	val = ((val << 16) & 0xFFFF0000FFFF0000ULL ) | ((val >> 16) & 0x0000FFFF0000FFFFULL );
	return (val << 32) | (val >> 32);
}

static void fwrite_u8_be(FILE* fh, uint8_t val){
	if (!is_big_endian()){
		val = bswap_u8(val);
	}
	fwrite(&val, sizeof(uint8_t), 1, fh);
}

static void fwrite_u16_be(FILE* fh, uint16_t val){
	if (!is_big_endian()){
		val = bswap_u16(val);
	}
	fwrite(&val, sizeof(uint16_t), 1, fh);
}

static void fwrite_u32_be(FILE* fh, uint32_t val){
	if (!is_big_endian()){
		val = bswap_u32(val);
	}
	fwrite(&val, sizeof(uint32_t), 1, fh);
}

static void fwrite_u64_be(FILE* fh, uint64_t val){
	if (!is_big_endian()){
		val = bswap_u64(val);
	}
	fwrite(&val, sizeof(uint64_t), 1, fh);
}
 
static void fwrite_float_be(FILE* fh, float val){
	uint32_t u = *((uint32_t*)&val);
	fwrite_u32_be(fh, u);
}

static void fwrite_double_be(FILE* fh, double val){
	uint64_t u = *((uint64_t*)&val);
	fwrite_u64_be(fh, u);
}

static void fwrite_strn_be(FILE* fh, const char* str, size_t len){
	for (size_t i=0; i < len; ++i){
		uint8_t c = *((uint8_t*)&str[i]);
		fwrite_u8_be(fh, c);
	}
}
static void fwrite_str_be(FILE* fh, const char* str){
	return fwrite_strn_be(fh, str, strlen(str));
}

static size_t component_type_size(cgltf_component_type t){
	switch(t){
		case cgltf_component_type_r_8: 
		case cgltf_component_type_r_8u:    return 1;
		case cgltf_component_type_r_16:
		case cgltf_component_type_r_16u:   return 2;
		case cgltf_component_type_r_32u:
		case cgltf_component_type_r_32f:   return 4;
		case cgltf_component_type_invalid:
		case cgltf_component_type_max_enum:
		default: return 0;
	}
}

bool str_starts_with(const char* str, const char *prefix)
{
    return strncmp(prefix, str, strlen(prefix)) == 0;
}

bool istr_starts_with(const char* str, const char *prefix)
{
    return strncasecmp(prefix, str, strlen(prefix)) == 0;
}

char* read_file(const char* filename){
	char* buffer = NULL;
	FILE* fh = fopen(filename, "rb");
	if (!fh){
		perror(filename);
		goto err;
	}
	fseek(fh, 0L, SEEK_END);
	size_t size = ftell(fh);
	rewind(fh);

	buffer = malloc(size+1);
	if (!buffer){
		goto err_file;
	}

	size_t bytes_read = fread(buffer, sizeof(char), size, fh);
	if (bytes_read < size){
		free(buffer);
		buffer = NULL;
		goto err_file;
	}
	buffer[bytes_read] = '\0';
err_file:
	fclose(fh);
err:
	return buffer;
}

static int jsmn_cmp(const char* json, const jsmntok_t* tok, const char* str){
	if ((int)strlen(str) == tok->end - tok->start &&
		strncmp(json+tok->start, str, tok->end - tok->start) == 0){
		return 0;
	}
	return -1;
}

static int jsmn_size(const char* js, const jsmntok_t* t, size_t remaining){
	if (remaining == 0){
		return 0;
	}
	int size = 0;
	if (t->type == JSMN_PRIMITIVE || t->type == JSMN_STRING){
		return size + 1;
	} else if (t->type == JSMN_OBJECT){
		for(size_t i=0; i < t->size; ++i){
			const jsmntok_t* key = t + 1 + size;
			size += jsmn_size(js, key, remaining - size);
			if (key->size > 0){
				size += jsmn_size(js, t + 1 + size, remaining - size);
			}
		}
		return size +1;
	} else if (t->type == JSMN_ARRAY){
		for (size_t i=0; i < t->size; ++i){
			size += jsmn_size(js, t + 1 + size, remaining - size);
		}
		return size + 1;
	}
	return size;
}
static int jsmn_find_key(const char* key, const char* js, jsmntok_t* tokens, size_t remaining){
	if (remaining == 0){
		return -1;
	}
	int idx = 0;
	while (idx < remaining){
		jsmntok_t* t = &tokens[idx];
		size_t left = remaining - idx;
		if (t->type != JSMN_OBJECT){
			idx += jsmn_size(js, t, left);
			continue;
		}
		size_t size = 0;
		for(size_t i=0; i < t->size; ++i){
			jsmntok_t* k = t + 1 + size;
			if (jsmn_cmp(js, k, key) == 0){
				return idx + size + 2;
			}
			size += jsmn_size(js, k, left - size);
			if (k->size > 0){
				size += jsmn_size(js, t+1+size, left - size);
			}
		}
		idx += size+1;
	}
	return -1;
}

static bool jsmn_check_bool_attribute(const char* js, const char* key){
	if (!js){
		return false;
	}
	jsmn_parser p;
	jsmn_init(&p);
	jsmntok_t tokens[MAX_EXTRA_JSON_TOKENS];
	int r = jsmn_parse(&p, js, strlen(js), &tokens[0], MAX_EXTRA_JSON_TOKENS);
	size_t remaining = p.toknext;

	bool ret = false;
	int token_key = jsmn_find_key(key, js, &tokens[0], remaining);
	if (token_key > 0){
		jsmntok_t* val = &tokens[token_key];
		ret = jsmn_cmp(js, val, "true") == 0;
	}
	return ret;
}

static bool is_decal(const char* js){
	return jsmn_check_bool_attribute(js, "decal");
}

static bool is_collider(const char* js){
	return jsmn_check_bool_attribute(js, "collider");
}

m44 walk_matrix_hierarchy(const cgltf_node* node, bool recursive){
	m44 scale = m44_identity();
	if (node->has_scale){
		m44_set_scale(&scale, &(vec3){{
			node->scale[0],
			node->scale[1],
			node->scale[2]
		}});
	}

	m44 rot = m44_identity();
	if (node->has_rotation){
		m44_set_rotation(&rot, &(quat){{
			node->rotation[0],
			node->rotation[1],
			node->rotation[2],
			node->rotation[3]
		}});
	}

	m44 trans = m44_identity();
	if (node->has_translation){
		m44_set_position(&trans, &(vec3){{
			node->translation[0],
			node->translation[1],
			node->translation[2]
		}});
	}
	m44 trans_rot = m44_mul_m44(&trans, &rot);
	m44 res = m44_mul_m44(&trans_rot, &scale);
	if (recursive && node->parent){
		m44 parent_mat = walk_matrix_hierarchy(node->parent, recursive);
		res = m44_mul_m44(&parent_mat, &res);
	}

	/* remove very small values */
	for (size_t i=0; i<4; ++i){
		for (size_t j=0; j<4; ++j){
			if (fabs(res.data[i][j]) < 0.0001f){
				res.data[i][j] = 0.0f;
			}
		}
	}

	return res;
}

static void apply_vertex_trans_rot_scale(vec3* vertex, const m44* matrix, float model_scale){
	vec3 mat_vert = m44_mul_vec3(matrix, vertex);
	*vertex = vec3_mul_float(&mat_vert, model_scale);
}

static bool get_mesh_aabb(const cgltf_mesh* mesh, const m44* matrix, float model_scale, float (*aabb_min)[3], float (*aabb_max)[3]){
	for(int j=0; j < mesh->primitives_count; ++j){
		cgltf_primitive* prim = &mesh->primitives[j];
		for(size_t k=0; k < prim->attributes_count; ++k){
			cgltf_attribute* attr = &prim->attributes[k];
			cgltf_accessor* acc = attr->data;
			uint8_t* base_ptr = ((uint8_t*)acc->buffer_view->buffer->data) + acc->buffer_view->offset + acc->offset;
			size_t elem_size = component_type_size(acc->component_type);
			if (attr->type == cgltf_attribute_type_position){
				if (attr->data->type != cgltf_type_vec3){
					fprintf(stderr, "Error: vertex position not vec3\n");
					return EXIT_FAILURE;
				}
				if (acc->component_type != cgltf_component_type_r_32f){
					fprintf(stderr, "Error: vertex position not float\n");
					return EXIT_FAILURE;
				}
				for (size_t l=0; l < acc->count; ++l){
					vec3 vertex = {0};
					for (size_t v=0; v<3; ++v){
						vertex.data[v] = *(float*)(base_ptr);
						base_ptr += elem_size;
					}
					apply_vertex_trans_rot_scale(&vertex, matrix, model_scale);

					for (size_t v=0; v<3; ++v){
						(*aabb_min)[v] = fminf((*aabb_min)[v], vertex.data[v]);
						(*aabb_max)[v] = fmaxf((*aabb_max)[v], vertex.data[v]);
					}
				}
			}
		}
	}
}

/* returns the offset into table where string exists */
static uint32_t append_string(char** table, size_t* table_len, const char* str){
	size_t len = strlen(str);
	char* start = *table;
	int remaining = *table_len;

	/* look for existing string */
	for(; remaining < 0;){
		char* found = memchr(start, str[0], remaining);
		if (found){
			remaining -= found - start;
			if (len > remaining){
				/* not found*/
				break;
			}
			if (memcmp(found, str, len) == 0){
				return found - *table;
			}
		}
		++start;
		--remaining;
	}
	/* need to append */
	size_t new_len = *table_len + len +1;
	char* new_table = malloc(new_len);
	memcpy(new_table, *table, *table_len);
	memcpy(new_table+*table_len, str, len);
	new_table[new_len-1] = '\0';
	free(*table);
	*table = new_table;
	uint32_t offset = *table_len;
	*table_len = new_len;
	return offset;
}

void help(FILE* stream){
	fprintf(stream, "usage: glb_to_lvl <input.glb> [--json input.json] <output.lvl>\n");
}

int main(int argc, char* argv[]){
	const char* glb_file = NULL; 
	const char* json_file = NULL;
	const char* lvl_file = NULL;

	for (size_t i=1; i < argc; ++i){
		char* arg = argv[i];
		if (strcmp(arg, "--json") == 0){
			if (i+1 >= argc){ 
				fprintf(stderr, "Error: Missing option after '%s'", arg);
				return EXIT_FAILURE;
			}
			json_file = argv[++i];
		} else if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0){
			help(stdout);
			return EXIT_SUCCESS;
		} else if (!glb_file){
			glb_file = arg;
		} else if (!lvl_file){
			lvl_file = arg;
		} else {
			fprintf(stderr, "Error: Unknown argument '%s'\n", arg);
			return EXIT_FAILURE;
		}
	}
	if (!glb_file || !lvl_file){
		help(stderr);
		return EXIT_FAILURE;
	}


	cgltf_options options = {0};
	cgltf_data* data = NULL;
	cgltf_result result = cgltf_parse_file(&options, glb_file, &data);

	if (result == cgltf_result_file_not_found){
		fprintf(stderr, "Error: File not found! (%s)\n", glb_file);
		return EXIT_FAILURE;
	}
	if (cgltf_validate(data) != cgltf_result_success){
		fprintf(stderr, "Invalid glTF data!\n");
		return EXIT_FAILURE;
	}
	cgltf_load_buffers(&options, data, glb_file);

	/* Chunks to export */
	lvl_chunk_camera_t camera_chunks[MAX_CAMERAS] = {0};
	size_t n_camera_chunks = 0;

	lvl_chunk_bounds_aabb_t bounds_chunks[MAX_BOUNDS] = {0};
	size_t n_bounds_chunks = 0;

	lvl_chunk_collider_aabb_t collider_chunks[MAX_COLLIDERS] = {0};

	size_t n_collider_chunks = 0;

	lvl_chunk_object_t object_chunks[MAX_OBJECTS] = {0};
	size_t n_object_chunks = 0;

	lvl_chunk_json_t json_chunks[MAX_JSON] = {0};
	size_t n_json_chunks = 0;
	if (json_file){
		lvl_chunk_json_t* j = &json_chunks[n_json_chunks++];
		j->json = read_file(json_file);
		if (!j->json){
			fprintf(stderr, "Error reading JSON data from '%s'!\n", json_file);
			return EXIT_FAILURE;
		}
	}

	lvl_chunk_light_t light_chunks[MAX_LIGHTS] = {0};
	size_t n_light_chunks = 0;

	lvl_chunk_zone_trigger_t zone_chunks[MAX_ZONES] = {0};
	size_t n_zone_chunks = 0;

	/* Loop through GLTF and convert to lvl chunk */
	for (size_t i=0; i < data->nodes_count; ++i){
		cgltf_node* node = &data->nodes[i];
		printf("Node %d: %s\n", i, node->name);

		m44 matrix = walk_matrix_hierarchy(node, true);

		if (node->camera){
			if (n_camera_chunks >= MAX_CAMERAS){
				fprintf(stderr, "Error: Too many cameras to export\n");
				return EXIT_FAILURE;
			}
			lvl_chunk_camera_t* cam = &camera_chunks[n_camera_chunks++];
			cam->name = node->name;
			vec3 pos = {0,0,0};
			vec3 tgt = {0,0,0};
			apply_vertex_trans_rot_scale(&pos, &matrix, MODEL_SCALE);
			memcpy(&cam->position[0], &pos.data[0], sizeof(float)*3);
			/*TODO target */
			memcpy(&cam->target[0], &tgt.data[0], sizeof(float)*3);
			printf("Camera\n\tpos: %f %f %f\n", pos.data[0], pos.data[1], pos.data[2]);
		}

		
		if (str_starts_with(node->name, "object_")){
			if (n_object_chunks >= MAX_OBJECTS){
				fprintf(stderr, "Error: Too many objects to export\n");
				return EXIT_FAILURE;
			}
			lvl_chunk_object_t* obj = &object_chunks[n_object_chunks++];
			obj->name = node->name + strlen("object_"); /*remove "object_" prefix*/
			vec3 pos = {0,0,0};
			apply_vertex_trans_rot_scale(&pos, &matrix, MODEL_SCALE);
			memcpy(&obj->position[0], &pos.data[0], sizeof(float)*3);
			obj->is_decal= is_decal(node->extras.data);
			printf("Object %s\n\tpos: %f %f %f\n\tis_decal: %s\n", 
				obj->name, 
				pos.data[0], pos.data[1], pos.data[2],
				(obj->is_decal ? "true" : "false")
			);
		}

		if (str_starts_with(node->name, "light_")){
			if (!node->light){
				fprintf(stderr, "Error: %s node is not a light\n", node->name);
				return EXIT_FAILURE;
			}
			if (n_light_chunks >= MAX_LIGHTS){
				fprintf(stderr, "Error: Too many lights to export\n");
				return EXIT_FAILURE;
			}
			lvl_chunk_light_t* light = &light_chunks[n_light_chunks++];
			light->name = node->name + strlen("light_"); /*remove "light_" prefix*/
			vec3 pos = {0,0,0};
			apply_vertex_trans_rot_scale(&pos, &matrix, MODEL_SCALE);
			memcpy(&light->position[0], &pos.data[0], sizeof(float)*3);
			memcpy(&light->color[0], &node->light->color[0], sizeof(float)*3);
			light->intensity = node->light->intensity;
			light->is_directional = (node->light->type == cgltf_light_type_directional) ? 1 : 0;
			light->enabled = 1;
			printf("Light %s\n", light->name);
			printf("\tpos: %f %f %f\n", pos.data[0], pos.data[1], pos.data[2]);
			printf("\tcolor: %f %f %f\n", light->color[0], light->color[1], light->color[2]);
			printf("\tintensity: %f\n", light->intensity);
			printf("\ttype: %s\n", (light->is_directional ? "directional" : "point"));
		}

		cgltf_mesh* mesh = node->mesh;
		if (!mesh){
			continue;
		}

		/* Check for bounds mesh */
		if (strcmp(mesh->name, "bounds") == 0){
			if (n_bounds_chunks >= MAX_BOUNDS){
				fprintf(stderr, "Error: Too many bounds to export\n");
				return EXIT_FAILURE;
			}
			lvl_chunk_bounds_aabb_t* bounds = &bounds_chunks[n_bounds_chunks++];
			*bounds = (lvl_chunk_bounds_aabb_t){
				{FLT_MAX,FLT_MAX,FLT_MAX},
				{-FLT_MAX,-FLT_MAX,-FLT_MAX}
			};
			if (!get_mesh_aabb(mesh, &matrix, MODEL_SCALE, &bounds->aabb_min, &bounds->aabb_max)){
				return EXIT_FAILURE;
			}
			printf("Bounds\n");
			printf("\taabb_min: %f %f %f\n", bounds->aabb_min[0], bounds->aabb_min[1], bounds->aabb_min[2]);
			printf("\taabb_max: %f %f %f\n", bounds->aabb_max[0], bounds->aabb_max[1], bounds->aabb_max[2]);
		}

		if (str_starts_with(mesh->name, "zone_")){
			if (n_zone_chunks >= MAX_ZONES){
				fprintf(stderr, "Error: Too many zone triggers to export\n");
				return EXIT_FAILURE;
			}
			lvl_chunk_zone_trigger_t* z = &zone_chunks[n_zone_chunks++];
			*z = (lvl_chunk_zone_trigger_t){
				NULL,
				{FLT_MAX,FLT_MAX,FLT_MAX},
				{-FLT_MAX,-FLT_MAX,-FLT_MAX},
				/*fire_once=*/1,
				/*fired_previously=*/0,
			};
			z->name = mesh->name;
			if (!get_mesh_aabb(mesh, &matrix, MODEL_SCALE, &z->aabb_min, &z->aabb_max)){
				return EXIT_FAILURE;
			}
			printf("Zone Trigger: %s\n", z->name);
			printf("\taabb_min: %f %f %f\n", z->aabb_min[0], z->aabb_min[1], z->aabb_min[2]);
			printf("\taabb_max: %f %f %f\n", z->aabb_max[0], z->aabb_max[1], z->aabb_max[2]);
		}

		/* Check if Node object has collider custom property */
		if (is_collider(node->extras.data)){
			if (n_collider_chunks >= MAX_COLLIDERS){
				fprintf(stderr, "Error: Too many colliders to export\n");
				return EXIT_FAILURE;
			}
			lvl_chunk_collider_aabb_t* col = &collider_chunks[n_collider_chunks++];
			*col = (lvl_chunk_collider_aabb_t){
				NULL,
				{FLT_MAX,FLT_MAX,FLT_MAX},
				{-FLT_MAX,-FLT_MAX,-FLT_MAX}
			};
			col->name = mesh->name;

			if (!get_mesh_aabb(mesh, &matrix, MODEL_SCALE, &col->aabb_min, &col->aabb_max)){
				return EXIT_FAILURE;
			}
			printf("Collider %s\n", col->name);
			printf("\taabb_min: %f %f %f\n", col->aabb_min[0], col->aabb_min[1], col->aabb_min[2]);
			printf("\taabb_max: %f %f %f\n", col->aabb_max[0], col->aabb_max[1], col->aabb_max[2]);
		}
	}

	size_t chunk_count = n_camera_chunks +
	                     n_bounds_chunks +
	                     n_collider_chunks + 
	                     n_object_chunks +
	                     n_json_chunks +
	                     n_light_chunks +
						 n_zone_chunks;

	FILE* lvl_fh = fopen(lvl_file, "wb");
	fwrite_str_be(lvl_fh, "LVL");
	fwrite_u8_be(lvl_fh, LVL_VERSION);

	fwrite_u32_be(lvl_fh, chunk_count);

	char* string_table = strdup("");
	size_t string_table_len = 1; /*null term included */
	size_t string_table_pos = ftell(lvl_fh);
	/* write empty string table offset */
	fwrite_u32_be(lvl_fh, 0);

	size_t chunk_table_pos = ftell(lvl_fh);
	/* write empty chunk table */
	for (size_t i=0; i < chunk_count; ++i){
		fwrite_u32_be(lvl_fh, 0);
	}

	/* append chunks */
	size_t chunk_idx = 0;

#define append_chunk(fh, type)                           \
	do {                                                 \
		size_t curr_offset = ftell(fh);                  \
		size_t curr_chunk_table_pos = chunk_table_pos +  \
		                  sizeof(uint32_t) * chunk_idx;  \
		fseek(fh, curr_chunk_table_pos, SEEK_SET);       \
		uint32_t chunk_data = curr_offset & 0xFFFFFF;    \
		chunk_data |= (uint32_t)type << 24;              \
		fwrite_u32_be(fh, chunk_data);                   \
		fseek(fh, curr_offset, SEEK_SET);                \
	} while(0)

	for (size_t i=0; i < n_camera_chunks; ++i){
		lvl_chunk_camera_t* cam = &camera_chunks[i];
		append_chunk(lvl_fh, LVL_CHUNK_TYPE_CAMERA);
		fwrite_u32_be(lvl_fh, append_string(&string_table, &string_table_len, cam->name));
		fwrite_float_be(lvl_fh, cam->position[0]);
		fwrite_float_be(lvl_fh, cam->position[1]);
		fwrite_float_be(lvl_fh, cam->position[2]);
		fwrite_float_be(lvl_fh, cam->target[0]);
		fwrite_float_be(lvl_fh, cam->target[1]);
		fwrite_float_be(lvl_fh, cam->target[2]);
		++chunk_idx;
	}

	for (size_t i=0; i < n_bounds_chunks; ++i){
		lvl_chunk_bounds_aabb_t* bounds = &bounds_chunks[i];
		append_chunk(lvl_fh, LVL_CHUNK_TYPE_BOUNDS_AABB);
		fwrite_float_be(lvl_fh, bounds->aabb_min[0]);
		fwrite_float_be(lvl_fh, bounds->aabb_min[1]);
		fwrite_float_be(lvl_fh, bounds->aabb_min[2]);
		fwrite_float_be(lvl_fh, bounds->aabb_max[0]);
		fwrite_float_be(lvl_fh, bounds->aabb_max[1]);
		fwrite_float_be(lvl_fh, bounds->aabb_max[2]);
		++chunk_idx;
	}

	for (size_t i=0; i < n_collider_chunks; ++i){
		lvl_chunk_collider_aabb_t* col = &collider_chunks[i];
		append_chunk(lvl_fh, LVL_CHUNK_TYPE_COLLIDER_AABB);
		fwrite_u32_be(lvl_fh, append_string(&string_table, &string_table_len, col->name));
		fwrite_float_be(lvl_fh, col->aabb_min[0]);
		fwrite_float_be(lvl_fh, col->aabb_min[1]);
		fwrite_float_be(lvl_fh, col->aabb_min[2]);
		fwrite_float_be(lvl_fh, col->aabb_max[0]);
		fwrite_float_be(lvl_fh, col->aabb_max[1]);
		fwrite_float_be(lvl_fh, col->aabb_max[2]);
		++chunk_idx;
	}

	for (size_t i=0; i < n_object_chunks; ++i){
		lvl_chunk_object_t* obj = &object_chunks[i];
		append_chunk(lvl_fh, LVL_CHUNK_TYPE_OBJECT);
		fwrite_u32_be(lvl_fh, append_string(&string_table, &string_table_len, obj->name));
		fwrite_float_be(lvl_fh, obj->position[0]);
		fwrite_float_be(lvl_fh, obj->position[1]);
		fwrite_float_be(lvl_fh, obj->position[2]);
		fwrite_u32_be(lvl_fh, obj->is_decal);
		++chunk_idx;
	}

	for (size_t i=0; i < n_json_chunks; ++i){
		lvl_chunk_json_t* j = &json_chunks[i];
		append_chunk(lvl_fh, LVL_CHUNK_TYPE_JSON);
		fwrite_u32_be(lvl_fh, append_string(&string_table, &string_table_len, j->json));
		++chunk_idx;
	}

	for (size_t i=0; i < n_light_chunks; ++i){
		lvl_chunk_light_t* light = &light_chunks[i];
		append_chunk(lvl_fh, LVL_CHUNK_TYPE_LIGHT);
		fwrite_u32_be(lvl_fh, append_string(&string_table, &string_table_len, light->name));
		fwrite_float_be(lvl_fh, light->position[0]);
		fwrite_float_be(lvl_fh, light->position[1]);
		fwrite_float_be(lvl_fh, light->position[2]);
		fwrite_float_be(lvl_fh, light->color[0]);
		fwrite_float_be(lvl_fh, light->color[1]);
		fwrite_float_be(lvl_fh, light->color[2]);
		fwrite_float_be(lvl_fh, light->intensity);
		fwrite_u32_be(lvl_fh, light->is_directional);
		fwrite_u32_be(lvl_fh, light->enabled);
		++chunk_idx;
	}

	for (size_t i=0; i < n_zone_chunks; ++i){
		lvl_chunk_zone_trigger_t* z = &zone_chunks[i];
		append_chunk(lvl_fh, LVL_CHUNK_TYPE_ZONE_TRIGGER);
		fwrite_u32_be(lvl_fh, append_string(&string_table, &string_table_len, z->name));
		fwrite_float_be(lvl_fh, z->aabb_min[0]);
		fwrite_float_be(lvl_fh, z->aabb_min[1]);
		fwrite_float_be(lvl_fh, z->aabb_min[2]);
		fwrite_float_be(lvl_fh, z->aabb_max[0]);
		fwrite_float_be(lvl_fh, z->aabb_max[1]);
		fwrite_float_be(lvl_fh, z->aabb_max[2]);
		fwrite_u32_be(lvl_fh, z->fire_once);
		fwrite_u32_be(lvl_fh, z->fired_previously);
		++chunk_idx;
	}

	/* String table */
	/*TODO: alignment? */
	uint32_t string_table_offset = ftell(lvl_fh);
	fwrite_strn_be(lvl_fh, string_table, string_table_len);
	size_t end = ftell(lvl_fh);
	fseek(lvl_fh, string_table_pos, SEEK_SET);
	fwrite_u32_be(lvl_fh, string_table_offset);
	fseek(lvl_fh, end, SEEK_SET);


	fclose(lvl_fh);

	return EXIT_SUCCESS;
}
