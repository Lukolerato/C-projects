#include "file_reader.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include "file_reader.h"
#include "tested_declarations.h"
#include "rdebug.h"
#include "tested_declarations.h"
#include "rdebug.h"

struct super_t super_sector;

struct disk_t* disk_open_from_file(const char* volume_file_name){
    if(volume_file_name == NULL){
        errno = EFAULT;
        return NULL;
    }

    FILE*  source = fopen(volume_file_name,"rb");
    if(source == NULL){
        errno = ENOENT;
        return NULL;
    }

    struct disk_t* result = malloc(sizeof(struct disk_t) );
    if(result == NULL){
        errno = ENOMEM;
        fclose(source);
        return NULL;
    }

    result->source = source;

    int temp = fread(&super_sector, sizeof(struct super_t),1,source);

    if(temp !=1 ){
        errno = EFAULT;
        fclose(source);
        free(result);
        return NULL;
    }


    return result;
}



int disk_read(struct disk_t* pdisk, int32_t first_sector, void* buffer, int32_t sectors_to_read){
    if(pdisk == NULL || buffer == NULL) {
        errno = EFAULT;
        return -1;
    }

    if(first_sector < 0 && sectors_to_read < 0){
        errno = EFAULT;
        return -1;
    }

    fseek(pdisk->source,first_sector*super_sector.bytes_per_sector,SEEK_SET);

    int temp = fread(buffer,super_sector.bytes_per_sector,sectors_to_read,pdisk->source);

    if(temp != sectors_to_read){
        errno = ERANGE;
        return -1;
    }
    else return temp;

}
int disk_close(struct disk_t* pdisk){
    if(pdisk != NULL){
        if(pdisk->source != NULL){
            fclose(pdisk->source);
            free(pdisk);
            return 0;
        }
    }
    errno = EFAULT;
    return -1;

}


struct volume_t* fat_open(struct disk_t* pdisk, uint32_t first_sector){
    if(pdisk == NULL){
        errno = EFAULT;
        return  NULL;
    }
    if(super_sector.magic != 0xaa55){//512
        errno = EFAULT;
        return  NULL;
    }

    struct volume_t* vol = malloc(sizeof(struct volume_t));
    if(vol == NULL){
        errno = ENOMEM;
        return NULL;
    }

    vol->fat1 = malloc(super_sector.sectors_per_fat*super_sector.bytes_per_sector);
    if(vol->fat1 == NULL){
        errno = ENOMEM;
        free(vol);
        return NULL;
    }

    uint32_t fat1_pos = first_sector+super_sector.reserved_sectors;
    int temp = disk_read(pdisk,fat1_pos,vol->fat1,super_sector.sectors_per_fat);
    if(temp != super_sector.sectors_per_fat){
        free(vol->fat1);
        free(vol);
        return NULL;
    }

    if(super_sector.fat_count == 1 || super_sector.fat_count == 2){
        if(super_sector.fat_count ==2){
            uint16_t *fat2 = malloc(super_sector.sectors_per_fat*super_sector.bytes_per_sector);
            if(fat2 == NULL){
                errno = ENOMEM;
                free(vol->fat1);
                return NULL;
            }
            uint16_t fat2_pos = fat1_pos + super_sector.sectors_per_fat;
            int temp2 = disk_read(pdisk,fat2_pos,fat2,super_sector.sectors_per_fat);
            if(temp2 != super_sector.sectors_per_fat){
                free(vol->fat1);
                free(fat2);
                free(vol);
                return NULL;
            }
            int temp3 = memcmp(vol->fat1,fat2,super_sector.sectors_per_fat*super_sector.bytes_per_sector);
            if(temp3 != 0){
                free(vol->fat1);
                free(fat2);
                free(vol);
                errno = EINVAL;
                return NULL;
            }
            free(fat2);
        }
        
        uint32_t rootdir_sectors = (super_sector.root_dir_capacity*sizeof(struct dir_entry_t))/super_sector.bytes_per_sector;
        if((super_sector.root_dir_capacity*sizeof(struct dir_entry_t))%2 != 0){
            rootdir_sectors += 1;
        }

        //uint32_t vol_size = super_sector.logical_sectors16==0 ? super_sector.logical_sectors32 : super_sector.logical_sectors16;
        //uint32_t user_size = vol_size - (super_sector.fat_count*super_sector.sectors_per_fat) - super_sector.reserved_sectors - rootdir_sectors;


        vol->rootdir_sectors = rootdir_sectors;
        vol->disc = (struct disk_t*)pdisk;
        vol->rootdir_position = super_sector.reserved_sectors+super_sector.fat_count*super_sector.sectors_per_fat;
        vol->rootdir_size = super_sector.root_dir_capacity;



        return vol;
    }
    free(vol->fat1);
    free(vol);
    errno = EINVAL;
    return NULL;
}

int fat_close(struct volume_t* pvolume){
    if(pvolume == NULL){
        errno = EFAULT;
        return -1;
    }
    if(pvolume->fat1 == NULL){
        errno = EFAULT;
        free(pvolume);
        pvolume = NULL;
        return -1;
    }
    if(pvolume->disc == NULL){
        errno = EFAULT;
        free(pvolume->fat1);
        free(pvolume);
        pvolume = NULL;
        return -1;
    }
    free(pvolume->fat1);
    free(pvolume);
    return 0;
}

struct file_t* file_open(struct volume_t* pvolume, const char* file_name){
    if(pvolume == NULL || file_name == NULL){
        errno = EFAULT;
        return NULL;
    }
    struct file_t *file = malloc(sizeof (struct file_t));
    strncpy(file->file_name,file_name,strlen(file_name));
    uint8_t *rootdir_content;
    uint32_t rootdir_sectors;
    rootdir_sectors = pvolume->rootdir_sectors;
    rootdir_content = malloc(rootdir_sectors*super_sector.bytes_per_sector);
    if(rootdir_content == NULL){
        errno = ENOMEM;
        return NULL;
    }
    uint32_t temp;
    temp = disk_read(pvolume->disc,pvolume->rootdir_position,rootdir_content,rootdir_sectors);

    if(temp != rootdir_sectors){
        errno = EINVAL;
        free(rootdir_content);
        free(file);
        return NULL;
    }
    struct fat_entry_t *entry = (struct fat_entry_t *) rootdir_content;
    int temp2 = 0;


    char name[14] = {0};
    char ext[4] = {0};
    char c[2] = {0};
    struct fat_entry_t *res;
    for(uint32_t i=0;i<pvolume->rootdir_size;i++){
        if(entry[i].name[0] == 0) break;
        if((entry[i].attr & attr_dir) != 0 || (entry[i].attr & attr_vol_label) != 0 ) continue;

        memset(name,0,13);
        memset(ext,0,3);

        for(int j =0; j<8 && !isspace(entry[i].name[j]);j++)
        {
            c[0] = entry[i].name[j];
            strcat(name,c);
        }
        for(int j =0; j<3 && !isspace(entry[i].ext[j]);j++)
        {
            c[0]= entry[i].ext[j];
            strcat(ext,c);
        }

        if(strlen(ext)>0 && strcmp(ext, "   ")!= 0){
            strcat(name,".");
            strcat(name,ext);
        }

        int temp4 = strncmp(name,file_name,strlen(file_name));


        if(temp4 == 0) {
            res = entry+i;
            temp2 = entry[i].low_chain_index;
            file->cluster_index = temp2;
            file->size = entry[i].size;
            file->disk = pvolume->disc;
            break;
        }
    }
    if(temp2 == 0){
        errno = ENOENT;
        free(rootdir_content);
        free(file);
        return NULL;
    }
    file->meta = res;
    uint32_t cluster_offset = super_sector.bytes_per_sector*super_sector.sectors_per_cluster;
    uint16_t clusters = res->size/cluster_offset;
    if(clusters*cluster_offset <= res->size){
        clusters++;
    }
    uint8_t *content = malloc(clusters*cluster_offset);
    if(content == NULL){
        errno = ENOMEM;
        free(rootdir_content);
        free(file);
        return NULL;
    }

    uint32_t data_offset = 0;
    int index = file->cluster_index;
    int data_area_pos = pvolume->rootdir_position + rootdir_sectors-1;
    int cluster;
    while(1){
        if(index>=0xfff8){break;}
        cluster= data_area_pos+(index-2)*super_sector.sectors_per_cluster;//-1
        int cn = disk_read(pvolume->disc,cluster,content+data_offset,super_sector.sectors_per_cluster);
        if(cn != super_sector.sectors_per_cluster){
            free(rootdir_content);
            free(file);
            free(content);
            return NULL;
        }
        index = pvolume->fat1[index];
        data_offset = data_offset+cluster_offset;

    }
    file->dane = content;
    file->position = 0;
    free(rootdir_content);
    return file;
}

int file_close(struct file_t* stream){
    if(stream == NULL){
        return -1;
    }
    free(stream->dane);
    free(stream);
    return 0;
}

size_t file_read(void *ptr, size_t size, size_t nmemb, struct file_t *stream){
    if(ptr == NULL || stream == NULL || size <1 || nmemb <1){
        errno = EINVAL;
        return -1;
    }
    if(stream->position>stream->size){
        return 0;
    }
    uint32_t count = 0;
    uint8_t *emp = (uint8_t*)ptr;
    if(stream->size - stream->position < size ){
        for(size_t i=0;i<size && stream->position+i<stream->size;i++){
            *(emp+i) = *(stream->dane+stream->position+i);
        }
        return 0;
    }
    for(size_t i =0,j=0;i<nmemb && stream->position<stream->size;){
        for(j=0;j<size && stream->position+j<stream->size;j++){
            *(emp+i+j) = *(stream->dane+stream->position+j);
        }
        i++;
        stream->position += j;
        count++;
    }
    return count;
}

int32_t file_seek(struct file_t* stream, int32_t offset, int whence){
    if(stream == NULL){
        errno = EFAULT;
        return -1;
    }
    int32_t temp;
    if(whence == SEEK_SET){
        if(offset >= (int32_t)stream->size || offset<0){
            errno = ENXIO;
            return -1;
        }
        else {
            stream->position = offset;
            return 0;
        }
    }
    else if(whence == SEEK_CUR){
        temp = stream->position+offset;
        if(temp>= (int32_t)stream->size || temp<0){
            errno = ENXIO;
            return -1;
        }
        else {
            stream->position = temp;
            return 0;
        }
    }
    else if(whence == SEEK_END){
        temp = stream->size+offset;
        if(temp>= (int32_t)stream->size || temp<0){
            errno = ENXIO;
            return -1;
        }
        else {
            stream->position = temp;
            return 0;
        }
    }
    else {
        errno = EINVAL;
        return -1;
    }
}

struct dir_t* dir_open(struct volume_t* pvolume, const char* dir_path){
    if(pvolume == NULL || dir_path == NULL){
        errno = EINVAL;
        return NULL;
    }
    if( strcmp(dir_path,"\\")){
        errno = ENOENT;
        return NULL;
    }
    uint32_t rootdir_sectors = (super_sector.root_dir_capacity*sizeof(struct dir_entry_t))/super_sector.bytes_per_sector;
    if((super_sector.root_dir_capacity*sizeof(struct dir_entry_t))%super_sector.bytes_per_sector != 0){
        rootdir_sectors += 1;
    }
    uint8_t *content = malloc(rootdir_sectors*super_sector.bytes_per_sector);
    if(content == NULL){
        errno = ENOMEM;
        return NULL;
    }
    uint32_t temp = disk_read(pvolume->disc,pvolume->rootdir_position,content,rootdir_sectors);
    if(temp == pvolume->rootdir_size){
        free(content);
        return NULL;
    }
    struct dir_t *dir = malloc(sizeof (struct dir_t));
    if(dir == NULL){
        free(content);
        errno = ENOMEM;
        return NULL;
    }
    dir->content = (struct fat_entry_t*)content;
    dir->volume = pvolume;
    dir->curr=0;
    return dir;
}
int dir_read(struct dir_t* pdir, struct dir_entry_t* pentry){
    if(pdir == NULL || pentry == NULL){
        errno = EINVAL;
        return -1;
    }
    struct fat_entry_t *temp = pdir->content;
    if(temp == NULL){
        errno = EINVAL;
        return -1;
    }
    char name[13]= {0};

    for(uint32_t i=pdir->curr;i<pdir->volume->rootdir_size;i++){
        if(temp[i].name[0] == 0){
            break;
        }
        if(temp[i].name[0] == (char)0xe5){
            continue;
        }
        if((temp[i].attr & attr_vol_label)!=0){
            continue;
        }
        if((temp[i].attr & attr_dir) !=0){
            for(int j=0; j<8 && temp[i].name[j]!=' '; j++){
                name[j]=temp[i].name[j];
            }
            strcpy(pentry->name,name);
            pentry->is_directory=1;
            pentry->meta = temp+i;
            if(pentry->meta->attr & attr_archived)
                pentry->is_archived=1;
            else
                pentry->is_archived=0;

            if(pentry->meta->attr & attr_hidden)
                pentry->is_hidden=1;
            else
                pentry->is_hidden=0;

            if(pentry->meta->attr & attr_read_only)
                pentry->is_readonly=1;
            else
                pentry->is_readonly=0;

            if(pentry->meta->attr & attr_system)
                pentry->is_system=1;
            else
                pentry->is_system=0;
            pdir->curr = i+1;
            return 0;
        }
        else {
            for(int j=0; j<8 && temp[i].name[j]!=' '; j++){
                name[j]=temp[i].name[j];
            }
            strcpy(pentry->name,name);
            memset(name,0, strlen(name));
            for(int j=0; j<3 && temp[i].ext[j]!=' '; j++){
                name[j]=temp[i].ext[j];
            }
            pentry->is_directory = 0;
            pentry->meta = temp+i;
            if(strcmp(name,"   ")!=0 && strlen(name)!=0){
                strcat(pentry->name,".");
                strcat(pentry->name,name);
            }
            if(pentry->meta->attr & attr_archived)
                pentry->is_archived=1;
            else
                pentry->is_archived=0;

            if(pentry->meta->attr & attr_hidden)
                pentry->is_hidden=1;
            else
                pentry->is_hidden=0;

            if(pentry->meta->attr & attr_read_only)
                pentry->is_readonly=1;
            else
                pentry->is_readonly=0;

            if(pentry->meta->attr & attr_system)
                pentry->is_system=1;
            else
                pentry->is_system=0;
            pdir->curr = i+1;
            return 0;
        }
    }
    return 1;
}

int dir_close(struct dir_t* pdir){
    if(pdir == NULL){
        errno = EINVAL;
        return -1;
    }
    else{
        free(pdir->content);
        free(pdir);
        return 0;
    }
}

