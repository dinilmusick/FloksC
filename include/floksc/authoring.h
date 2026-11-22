#ifndef FLOKSC_AUTHORING_H
#define FLOKSC_AUTHORING_H
#include "floksc.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Renderer-free version of the Floks TypeScript authoring session contracts.
 * All arrays and ancestry scratch memory belong to the caller.
 * Functions do not allocate, retain borrowed strings or schedule work. */
#define FC_AUTHOR_MAX_IO 4
typedef enum {FC_AUTHOR_SERVICE=1,FC_AUTHOR_GENERAL_COMPONENT,FC_AUTHOR_APPLICATION} fc_author_context;
typedef enum {FC_AUTHOR_PAGE=1,FC_AUTHOR_SECTION,FC_AUTHOR_COMPONENT} fc_author_kind;
typedef enum {FC_AUTHOR_STRING=1,FC_AUTHOR_NUMBER,FC_AUTHOR_BOOLEAN} fc_author_value_kind;
typedef struct {
 fc_author_value_kind kind;
 char text[FC_TEXT];
 double number;
 bool boolean;
} fc_author_value;
typedef struct {
 char package_id[FC_NAME],package_version[FC_NAME],export_path[FC_TEXT];
 char asset_id[FC_NAME],asset_version[FC_NAME];
} fc_author_asset;
typedef struct {
 char id[FC_NAME],parent_id[FC_NAME],name[FC_TEXT],component_id[FC_NAME];
 fc_author_kind kind;
 fc_author_asset asset;
} fc_author_node;
typedef enum {
 FC_AUTHOR_PROPERTY=1,FC_AUTHOR_STATE_PROPERTY,FC_AUTHOR_EVENT,FC_AUTHOR_SLOT,
 FC_AUTHOR_BINDING,FC_AUTHOR_METADATA
} fc_author_field_kind;
typedef struct {
 fc_author_field_kind kind;
 char node_id[FC_NAME],key[FC_NAME],selector[FC_NAME];
 fc_author_value value;
 char target_kind[FC_NAME],target_api[FC_NAME],target_field[FC_NAME],target_type[FC_NAME];
} fc_author_field;
typedef struct {char name[FC_NAME],type[FC_NAME];} fc_author_io;
typedef struct {
 char id[FC_NAME],name[FC_TEXT],description[FC_TEXT];
 fc_author_io inputs[FC_AUTHOR_MAX_IO],outputs[FC_AUTHOR_MAX_IO];
 size_t input_count,output_count;
} fc_author_operation;
typedef struct {
 char session_id[FC_NAME];
 fc_author_context context;
 fc_author_node *nodes;
 size_t node_count,node_cap;
 fc_author_field *fields;
 size_t field_count,field_cap;
 fc_author_operation *operations;
 size_t operation_count,operation_cap;
 uint8_t *scratch; /* node_cap bytes, separate from all other storage */
} fc_author_session;
fc_result fc_author_init(fc_author_session *s,const char *session_id,fc_author_context context,
 fc_author_node *nodes,size_t node_cap,fc_author_field *fields,size_t field_cap,
 fc_author_operation *operations,size_t operation_cap,uint8_t *scratch);
const fc_author_node *fc_author_find(const fc_author_session *s,const char *id);
const fc_author_field *fc_author_field_find(const fc_author_session *s,
 fc_author_field_kind kind,const char *node,const char *key,const char *selector);
fc_result fc_author_node_add(fc_author_session *s,const char *id,fc_author_kind kind,
 const char *parent,const char *name,const char *component_id);
fc_result fc_author_node_update(fc_author_session *s,const char *id,
 const char *new_parent,const char *new_name);
fc_result fc_author_asset_set(fc_author_session *s,const char *node_id,fc_author_asset asset);
fc_result fc_author_node_remove(fc_author_session *s,const char *id);
fc_result fc_author_property(fc_author_session *s,const char *node,const char *key,fc_author_value value);
fc_result fc_author_state_property(fc_author_session *s,const char *node,const char *state,
 const char *key,fc_author_value value);
fc_result fc_author_event(fc_author_session *s,const char *node,const char *event,const char *handler);
fc_result fc_author_slot(fc_author_session *s,const char *node,const char *slot,const char *child);
fc_result fc_author_unslot(fc_author_session *s,const char *node,const char *slot,const char *child);
fc_result fc_author_bind(fc_author_session *s,const char *node,const char *property,
 const char *kind,const char *api_id,const char *field,const char *field_type);
fc_result fc_author_unbind(fc_author_session *s,const char *node,const char *property);
fc_result fc_author_metadata(fc_author_session *s,const char *key,const char *value);
fc_result fc_author_operation_set(fc_author_session *s,fc_author_operation op);
fc_result fc_author_operation_remove(fc_author_session *s,const char *id);
fc_result fc_author_validate(const fc_author_session *s);
/* FCAS v1 snapshot. Format uses explicit little-endian integers, version magic,
 * binary64 finite numbers and length-prefixed text. For required length pass
 * out=NULL; caller allocates output and separate decode scratch arrays.
 * Decode commits only after full graph/schema/field validation. */
fc_result fc_author_save(const fc_author_session *s,uint8_t *out,size_t capacity,size_t *written);
fc_result fc_author_load(fc_author_session *target,const uint8_t *data,size_t length,
 fc_author_node *scratch_nodes,fc_author_field *scratch_fields,
 fc_author_operation *scratch_operations,uint8_t *scratch_walk);
#ifdef __cplusplus
}
#endif
#endif
