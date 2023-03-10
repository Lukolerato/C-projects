#pragma once

#include <stdint.h>
#include <stdint.h>
#include <stdio.h>

enum file_attr
{
    attr_read_only = 0x01,
    attr_hidden = 0x02,
    attr_system = 0x04,
    attr_vol_label = 0x08,
    attr_dir = 0x10,
    attr_archived = 0x20,
};

struct disk_t{
    FILE* source ;
};

struct super_t{
    uint8_t jump_code[3];
    char oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t root_dir_capacity;
    uint16_t logical_sectors16;
    uint8_t media_type;
    uint16_t sectors_per_fat;
    uint16_t chs_sectors_per_track;
    uint16_t chs_tracks_per_cylinder;
    uint32_t hidden_sectors;
    uint32_t logical_sectors32;
    uint8_t media_id;
    uint8_t chs_head;
    uint8_t ext_bpb_signature;
    uint32_t serial_number;
    char volume_label[11];
    char fsid[8];
    uint8_t boot_code[448];
    uint16_t magic;
} __attribute__((__packed__));

struct volume_t{
    struct disk_t* disc;
    uint16_t *fat1;
    uint32_t rootdir_position;
    uint32_t rootdir_size;
    uint32_t rootdir_sectors;

};

struct file_t{
    char file_name[13];
    size_t size;
    uint32_t position;
    uint8_t *dane;
    struct disk_t *disk;
    int cluster_index;
    struct fat_entry_t* meta;
};

struct fat_entry_t
{
    char name[8];
    char ext[3];
    uint8_t attr;

    uint8_t __reserved0;
    uint8_t cration_time_ms;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_time;
    uint16_t high_chain_index;
    uint16_t last_modification_time;
    uint16_t last_modification_date;
    uint16_t low_chain_index;
    uint32_t size;

}__attribute__((packed));

struct dir_t{
    struct fat_entry_t *content;
    struct volume_t *volume;
    uint16_t curr;
};

struct dir_entry_t {
    char name[13]; //- nazwa pliku/katalogu bez nadmiarowych spacji oraz z kropką separującą nazwę od rozszerzenia,
    int is_archived; //- wartość atrybutu: plik zarchiwizowany (0 lub 1),
    int is_readonly; //- wartość atrybutu: plik tylko do odczytu (0 lub 1),
    int is_system; //- wartość atrybutu: plik jest systemowy (0 lub 1),
    int is_hidden; //- wartość atrybutu: plik jest ukryty (0 lub 1),
    int is_directory; //- wartość atrybutu: katalog (0 lub 1).
    struct fat_entry_t *meta;
};


struct clusters_chain_t {
    uint16_t *clusters;
    size_t size;
};


struct disk_t* disk_open_from_file(const char* volume_file_name);
int disk_read(struct disk_t* pdisk, int32_t first_sector, void* buffer, int32_t sectors_to_read);
int disk_close(struct disk_t* pdisk);

//struct clusters_chain_t *get_chain_fat12(const void * const buffer, size_t size, uint16_t first_cluster);
//struct clusters_chain_t *get_chain_fat16(const void * const buffer, size_t size, uint16_t first_cluster);

struct volume_t* fat_open(struct disk_t* pdisk, uint32_t first_sector);
int fat_close(struct volume_t* pvolume);

struct file_t* file_open(struct volume_t* pvolume, const char* file_name);
int file_close(struct file_t* stream);
size_t file_read(void *ptr, size_t size, size_t nmemb, struct file_t *stream);
int32_t file_seek(struct file_t* stream, int32_t offset, int whence);

struct dir_t* dir_open(struct volume_t* pvolume, const char* dir_path);
int dir_read(struct dir_t* pdir, struct dir_entry_t* pentry);
int dir_close(struct dir_t* pdir);