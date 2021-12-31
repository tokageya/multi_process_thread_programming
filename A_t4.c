/**********************************\
 A_t4.c
 $ gcc -Wall -std=c11 -pthread A_t1.c -ljpeg
\**********************************/
#define _DEFAULT_SOURCE // timersubのために必要？

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <jpeglib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h> // sleepに必要
#include <pthread.h> // pthreadのために必要
#include <sys/time.h>

#define MAX_PATH_LEN 1000
#define MAX_FILE_NUM 1000
#define THREAD_NUM 4 // スレッドの数

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



struct timeval t0;
struct timeval t1;
struct timeval t2;

char input_jpg_file_name_list[MAX_FILE_NUM][MAX_PATH_LEN];
char output_jpg_file_name_list[MAX_FILE_NUM][MAX_PATH_LEN];

int img_file_count = 0;


void *thread(void *args){ // スレッドに実行させる関数
    int* j = (int *)args;
    for(int i = 0 + ((*j)*((img_file_count/THREAD_NUM) + 1)); i < (((*j)+1)*((img_file_count/THREAD_NUM) + 1)) &&  i < img_file_count; i++){
        JpegData jpegData;
        struct jpeg_error_mgr jerr;
        if (!read_jpeg(&jpegData, input_jpg_file_name_list[i], &jerr)){
            fprintf(stderr, "ERROR: in function \"read_jpeg\", input_jpg_file_name_list[%d] : \"%s\".\n", i, input_jpg_file_name_list[i]);
            free_jpeg(&jpegData);
            return NULL;
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
            return NULL;
        }

        #ifdef _DEBUG
        printf("Write: %s\n", dst);
        printf("img_count : %d\n",img_count);
        #endif

        free_jpeg(&jpegData);
    }
    
    struct timeval t1;
    gettimeofday(&t1, NULL); // スレッドの実行時刻
    timersub(&t1, &t0, &t1);
    printf("subthread[%d]: %ld.%06ld\n",*j, t1.tv_sec, t1.tv_usec);
    return NULL;
}


int main(void){
    DIR *dir;
    struct dirent *dp;
    char dirpath[MAX_PATH_LEN] = "/home/msait/Downloads/all_jpg/";
    
    dir = opendir(dirpath);
    if (dir == NULL) { return 1; }

    dp = readdir(dir);
    
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
    
    gettimeofday(&t0, NULL);

    pthread_t th_0, th_1, th_2, th_3;
    int j0 = 0;
    pthread_create( &th_0, NULL, thread, (void *) &j0);
    int j1 = 1;
    pthread_create( &th_1, NULL, thread, (void *) &j1);
    int j2 = 2;
    pthread_create( &th_2, NULL, thread, (void *) &j2);
    int j3 = 3;
    pthread_create( &th_3, NULL, thread, (void *) &j3);
    
    pthread_join(th_0, NULL); // スレッドの終了待ち
    pthread_join(th_1, NULL); // スレッドの終了待ち
    pthread_join(th_2, NULL); // スレッドの終了待ち
    pthread_join(th_3, NULL); // スレッドの終了待ち

    
    
    gettimeofday(&t2, NULL);
    timersub(&t2, &t0, &t2);
    printf("time: %d.%06d\n", (int)t2.tv_sec, (int)t2.tv_usec);

    if (dir != NULL) closedir(dir);
    exit(EXIT_SUCCESS);
}