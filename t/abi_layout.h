#ifndef MMDB_ABI_LAYOUT_H
#define MMDB_ABI_LAYOUT_H

#include <stddef.h>

/* Expand against both current and frozen declarations in separate files. */
#define MMDB_ABI_LAYOUT(SIZE, FIELD)                                           \
    SIZE(MMDB_s)                                                               \
    FIELD(MMDB_s, flags)                                                       \
    FIELD(MMDB_s, filename)                                                    \
    FIELD(MMDB_s, file_size)                                                   \
    FIELD(MMDB_s, file_content)                                                \
    FIELD(MMDB_s, data_section)                                                \
    FIELD(MMDB_s, data_section_size)                                           \
    FIELD(MMDB_s, metadata_section)                                            \
    FIELD(MMDB_s, metadata_section_size)                                       \
    FIELD(MMDB_s, full_record_byte_size)                                       \
    FIELD(MMDB_s, depth)                                                       \
    FIELD(MMDB_s, ipv4_start_node)                                             \
    FIELD(MMDB_s, metadata)                                                    \
    SIZE(MMDB_metadata_s)                                                      \
    FIELD(MMDB_metadata_s, node_count)                                         \
    FIELD(MMDB_metadata_s, record_size)                                        \
    FIELD(MMDB_metadata_s, ip_version)                                         \
    FIELD(MMDB_metadata_s, database_type)                                      \
    FIELD(MMDB_metadata_s, languages)                                          \
    FIELD(MMDB_metadata_s, languages.count)                                    \
    FIELD(MMDB_metadata_s, languages.names)                                    \
    FIELD(MMDB_metadata_s, binary_format_major_version)                        \
    FIELD(MMDB_metadata_s, binary_format_minor_version)                        \
    FIELD(MMDB_metadata_s, build_epoch)                                        \
    FIELD(MMDB_metadata_s, description)                                        \
    FIELD(MMDB_metadata_s, description.count)                                  \
    FIELD(MMDB_metadata_s, description.descriptions)                           \
    SIZE(MMDB_ipv4_start_node_s)                                               \
    FIELD(MMDB_ipv4_start_node_s, netmask)                                     \
    FIELD(MMDB_ipv4_start_node_s, node_value)                                  \
    SIZE(MMDB_description_s)                                                   \
    FIELD(MMDB_description_s, language)                                        \
    FIELD(MMDB_description_s, description)                                     \
    SIZE(MMDB_entry_s)                                                         \
    FIELD(MMDB_entry_s, mmdb)                                                  \
    FIELD(MMDB_entry_s, offset)                                                \
    SIZE(MMDB_lookup_result_s)                                                 \
    FIELD(MMDB_lookup_result_s, found_entry)                                   \
    FIELD(MMDB_lookup_result_s, entry)                                         \
    FIELD(MMDB_lookup_result_s, netmask)                                       \
    SIZE(MMDB_entry_data_s)                                                    \
    FIELD(MMDB_entry_data_s, has_data)                                         \
    FIELD(MMDB_entry_data_s, pointer)                                          \
    FIELD(MMDB_entry_data_s, utf8_string)                                      \
    FIELD(MMDB_entry_data_s, double_value)                                     \
    FIELD(MMDB_entry_data_s, bytes)                                            \
    FIELD(MMDB_entry_data_s, uint16)                                           \
    FIELD(MMDB_entry_data_s, uint32)                                           \
    FIELD(MMDB_entry_data_s, int32)                                            \
    FIELD(MMDB_entry_data_s, uint64)                                           \
    FIELD(MMDB_entry_data_s, uint128)                                          \
    FIELD(MMDB_entry_data_s, boolean)                                          \
    FIELD(MMDB_entry_data_s, float_value)                                      \
    FIELD(MMDB_entry_data_s, offset)                                           \
    FIELD(MMDB_entry_data_s, offset_to_next)                                   \
    FIELD(MMDB_entry_data_s, data_size)                                        \
    FIELD(MMDB_entry_data_s, type)

const size_t *legacy_abi_layout(size_t *count);
int legacy_abi_client(const char *path);

#endif
