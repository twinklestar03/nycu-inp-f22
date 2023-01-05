#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

//#define DEBUG 0


struct PAKO_HDR {
    char magic[4];
    uint32_t string_section_offset;
    uint32_t content_secion_offset;
    uint32_t len_files;
};

struct FILE_E {
    uint32_t filename_offset;
    uint32_t filesize;
    uint32_t content_offset;
    char checksum[8];
};

uint32_t convert_indian_u32(uint32_t num) {
    uint32_t ret = 0;
    ret |= (num & 0x000000ff) << 24;
    ret |= (num & 0x0000ff00) << 8;
    ret |= (num & 0x00ff0000) >> 8;
    ret |= (num & 0xff000000) >> 24;
    return ret;
}

uint64_t convert_indian_u64(uint64_t num) {
    uint64_t ret = 0;
    ret |= (num & 0x00000000000000ff) << 56;
    ret |= (num & 0x000000000000ff00) << 40;
    ret |= (num & 0x0000000000ff0000) << 24;
    ret |= (num & 0x00000000ff000000) << 8;
    ret |= (num & 0x000000ff00000000) >> 8;
    ret |= (num & 0x0000ff0000000000) >> 24;
    ret |= (num & 0x00ff000000000000) >> 40;
    ret |= (num & 0xff00000000000000) >> 56;
    return ret;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Argument Error\n");
        return 0;
    }

    int fp = open(argv[1], O_RDONLY);
    if (fp < 0) {
        printf("Open Error \n");
        return 0;
    }

    struct PAKO_HDR hdr;
    int b_read = read(fp, &hdr, sizeof(struct PAKO_HDR));
    // Dump parsed hdr
    #ifdef DEBUG
    printf("[DEBUG] Magic: %s\n", hdr.magic);
    printf("[DEBUG] String Section Offset: %x\n", hdr.string_section_offset);
    printf("[DEBUG] Content Section Offset: %x\n", hdr.content_secion_offset);
    printf("[DEBUG] Len Files: %x\n", hdr.len_files);
    printf("[DEBUG] ----------------------------------\n");
    #endif

    // Parse file entries
    struct FILE_E* file_entries = (struct FILE_E*)malloc(hdr.len_files * sizeof(struct FILE_E));
    off_t off = lseek(fp, sizeof(struct PAKO_HDR), SEEK_SET);
    b_read = read(fp, file_entries, hdr.len_files * sizeof(struct FILE_E));

    // Coverting indian
    for (int i = 0; i < hdr.len_files; ++i) {
        file_entries[i].filesize = convert_indian_u32(file_entries[i].filesize);
        *((uint64_t *) file_entries[i].checksum) = convert_indian_u64( *((uint64_t *) file_entries[i].checksum) );
    }
    
    // Read and output files
    for (int i = 0; i < hdr.len_files; ++i) {
        // Read filename
        char* filename = (char*)malloc(256);
        off = lseek(fp, hdr.string_section_offset + file_entries[i].filename_offset, SEEK_SET);
        
        for (int j = 0; j < 256; ++j) {
            read(fp, &filename[j], 1);
            if (filename[j] == 0) {
                break;
            }
        }
        
        #ifdef DEBUG
        printf("[DEBUG] Extracted Filename: %s\n", filename);
        printf("[DEBUG] File Entry %d\n", i);
        printf("[DEBUG] Filename Offset: %x\n", file_entries[i].filename_offset);
        printf("[DEBUG] Filesize: %x\n", file_entries[i].filesize);
        printf("[DEBUG] Content Offset: %x\n", file_entries[i].content_offset);
        // dump checksum
        printf("[DEBUG] Checksum: %lx\n",  *((uint64_t *) file_entries[i].checksum));
        #endif

        // Read file content
        char* file_content = (char*)calloc(file_entries[i].filesize + 8 - file_entries[i].filesize % 8, 1);
        off = lseek(fp, hdr.content_secion_offset + file_entries[i].content_offset, SEEK_SET);
        read(fp, file_content, file_entries[i].filesize);

        // Do Checksum
        uint64_t checksum = 0;
        // split file into segments by 8 bytes, xor each segment
        int num_segments = ceil(file_entries[i].filesize / 8) + 1;
        int remainder = file_entries[i].filesize % 8;
        for (int n_seg = 0; n_seg < num_segments; ++n_seg) {
            uint64_t segment = 0;
            memcpy(&segment, file_content + n_seg * 8, 8);
            printf("0x%lx\n", segment);

            checksum ^= segment;
        }
        printf("[DEBUG] Calced Checksum: %lx\n", checksum);

        if (checksum != *((uint64_t *) file_entries[i].checksum)) {
            printf("[Extract] Checksum Error when extracting %s\n", filename);
            continue;
        }

        // Write to file at destination
        char* dest_filename = (char*)malloc(strlen(argv[2]) + strlen(filename) + 2);
        strcpy(dest_filename, argv[2]);
        strcat(dest_filename, "/");
        strcat(dest_filename, filename);
        int dst_fp = open(dest_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (dst_fp < 0) {
            printf("[Extract] Open Error when extracting %s\n", filename);
            continue;
        }
        write(dst_fp, file_content, file_entries[i].filesize);
        close(dst_fp);

        free(filename);
        free(file_content);
        free(dest_filename);

        #ifdef DEBUG
        printf("[DEBUG] ----------------------------------\n");
        #endif
    }
    close(fp);
    
    return 0;
}
