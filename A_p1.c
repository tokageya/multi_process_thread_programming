/*********************************\
 A_p1.c
 $ gcc -Wall -std=c11 A_p1.c -ljpeg
\*********************************/
#define _DEFAULT_SOURCE // timersubのために必要？

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <jpeglib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h> // forkのために必要
#include <unistd.h> // forkのために必要
#include <err.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/wait.h>

#define MAX_PATH_LEN 1000
#define MAX_FILE_NUM 1000

/* JpegData : jpeg用の構造体 */
typedef struct {
    uint8_t *data;   // raw data
    uint32_t width;
    uint32_t height;
    uint32_t ch;     // color channels
} JpegData;

/* alloc_jpeg : RGBデータ用の領域を確保 */
void alloc_jpeg(JpegData *jpegData){
    jpegData->data = NULL;
    jpegData->data = (uint8_t*) malloc(sizeof(uint8_t) * jpegData->width * jpegData->height * jpegData->ch);
}

/* free_jpeg : RGBデータ用の領域を開放 */
void free_jpeg(JpegData *jpegData){
    if (jpegData->data != NULL) {
        free(jpegData->data);
        jpegData->data = NULL;
    }
}

/* read_jpeg : JPEG画像を読み込む */
bool read_jpeg(JpegData *jpegData, const char *srcfile, struct jpeg_error_mgr *jerr){
    // 構造体確保
    struct jpeg_decompress_struct cinfo;
    jpeg_create_decompress(&cinfo);
    cinfo.err = jpeg_std_error(jerr);

    // 入力ファイル指定
    FILE *fp = fopen(srcfile, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Error: failed to open %s\n", srcfile);
        return false;
    }
    jpeg_stdio_src(&cinfo, fp);

    // ヘッダ情報取得
    jpeg_read_header(&cinfo, TRUE);

    // データ情報読み込み
    jpeg_start_decompress(&cinfo);
    jpegData->width  = cinfo.image_width;
    jpegData->height = cinfo.image_height;
    jpegData->ch     = cinfo.num_components;

    // 画像データ格納する配列を動的確保
    alloc_jpeg(jpegData);

    // 列単位でデータを格納
    uint8_t *row = jpegData->data;
    const uint32_t stride = jpegData->width * jpegData->ch;
    for (int y = 0; y < jpegData->height; y++) {
        jpeg_read_scanlines(&cinfo, &row, 1);
        row += stride;
    }

    // 構造体を破棄, ファイルをクローズ
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(fp);

    return true;
}

/* write_jpeg : jpeg画像を書き込む */
bool write_jpeg(const JpegData *jpegData, const char *dstfile, struct jpeg_error_mgr *jerr){
    // 構造体確保
    struct jpeg_compress_struct cinfo;
    jpeg_create_compress(&cinfo);
    cinfo.err = jpeg_std_error(jerr);

    // 出力ファイル指定
    FILE *fp = fopen(dstfile, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Error: failed to open %s\n", dstfile);
        return false;
    }
    jpeg_stdio_dest(&cinfo, fp);

    // ヘッダ情報を指定
    cinfo.image_width      = jpegData->width;
    cinfo.image_height     = jpegData->height;
    cinfo.input_components = jpegData->ch;
    cinfo.in_color_space   = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_start_compress(&cinfo, TRUE);

    // データ情報書き込み
    uint8_t *row = jpegData->data;
    const uint32_t stride = jpegData->width * jpegData->ch;
    for (int y = 0; y < jpegData->height; y++) {
        jpeg_write_scanlines(&cinfo, &row, 1);
        row += stride;
    }

    // 構造体を破棄, ファイルをクローズ
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(fp);

    return true;
}



int main(void){
    DIR *dir;
    struct dirent *dp;
    char dirpath[MAX_PATH_LEN] = "/home/msait/Downloads/all_jpg/";
    char input_jpg_file_name_list[MAX_FILE_NUM][MAX_PATH_LEN];
    char output_jpg_file_name_list[MAX_FILE_NUM][MAX_PATH_LEN];

    // 前準備：jpgファイルのリストを読み込む
    dir = opendir(dirpath);
    if (dir == NULL) { return 1; }

    dp = readdir(dir);
    int img_file_count = 0;

    while (dp != NULL) {
        if( strcmp(dp->d_name, "..") && strcmp(dp->d_name, ".") ){
            char src[MAX_PATH_LEN];
            strcpy(src, dirpath);
            strcat(src, dp->d_name);
            strcpy(input_jpg_file_name_list[img_file_count], src);
            char dst[MAX_PATH_LEN] = "/home/msait/Downloads/all_output_jpg/";
            strcat(dst, dp->d_name);
            strcpy(output_jpg_file_name_list[img_file_count], dst);
            #ifdef _DEBUG
            printf("%s\ninput: \"%s\"\noutput: \"%s\"\n", dp->d_name, src, dst);
            #endif
            img_file_count ++;
        }
        dp = readdir(dir);
    }
    #ifdef _DEBUG
    printf("img_file_count : %d\n", img_file_count);
    #endif


    /* img処理開始 */
    struct timeval t0;
    struct timeval t1;
    struct timeval t2;
    gettimeofday(&t0, NULL);

    pid_t pid;
    pid = fork();
    if (-1 == pid) {
        err (EXIT_FAILURE, "can not create");
    }
    else if (0 == pid) { // 子プロセス
        for(int i = 0; i < img_file_count; i++){
            JpegData jpegData;
            struct jpeg_error_mgr jerr;
            if (!read_jpeg(&jpegData, input_jpg_file_name_list[i], &jerr)){
                fprintf(stderr, "ERROR: in function \"read_jpeg\", input_jpg_file_name_list[%d]) : \"%s\".\n", i, input_jpg_file_name_list[i]);
                free_jpeg(&jpegData);
                return -1;
            }
            #ifdef _DEBUG
            printf("Read: %s\n", src);
            #endif

            // reverse all bits
            int size = jpegData.width * jpegData.height * jpegData.ch;
            for (int i = 0; i < size; i++) {
                jpegData.data[i] = ~jpegData.data[i];
            }

            if (!write_jpeg(&jpegData, output_jpg_file_name_list[i], &jerr)){
                fprintf(stderr, "ERROR: in function \"write_jpeg\".\n");
                free_jpeg(&jpegData);
                return -1;
            }

            #ifdef _DEBUG
            printf("Write: %s\n", dst);
            printf("img_count : %d\n",img_count);
            #endif
            
            free_jpeg(&jpegData);
        }
        gettimeofday(&t1, NULL);
        timersub(&t1, &t0, &t1);
        printf("child: %ld.%06ld\n", t1.tv_sec, t1.tv_usec);
        exit(EXIT_SUCCESS);
    }
    
    int status;
    wait(&status);
    gettimeofday(&t2, NULL);
    timersub(&t2, &t0, &t2);
    printf("parent: %ld.%06ld\n", t2.tv_sec, t2.tv_usec);

    if (dir != NULL) closedir(dir);
    exit(EXIT_SUCCESS);

}