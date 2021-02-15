/*
  read Wav file
  see http://hooktail.org/computer/index.php?Wave%A5%D5%A5%A1%A5%A4%A5%EB%A4%F2%C6%FE%BD%D0%CE%CF%A4%B7%A4%C6%A4%DF%A4%EB
waveファイルはデータをリトルエンディアンで保存している
このプログラムはCPUがリトルエンディアンでないと動作しない
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>

#define HEADERSIZE 44

typedef struct {
  int16_t l;
  int16_t r;
} Soundsample16;

typedef struct {
  uint8_t l;
  uint8_t r;
} Soundsample8;

typedef struct {
  uint16_t channelnum;      //モノラルなら1、ステレオなら2
  uint32_t samplingrate;     //Hz単位
  uint16_t bit_per_sample;    //1サンプルあたりのbit数
  uint32_t datanum;        //モノラルならサンプル数を、ステレオなら左右１サンプルずつの組の数

  uint8_t *monaural8;     //8ビットモノラルのデータならこれを使う
  int16_t *monaural16;     //16ビットモノラルならばこれを使う
  Soundsample8 *stereo8;        //8ビットステレオならばこれを使う
  Soundsample16 *stereo16;      //16ビットステレオならばこれを使う
} Sound;

//取得に成功すればポインタを失敗すればNULLを返す
Sound *Read_Wave(char *filename);

//書き込みに成功すれば0を失敗すれば1を返す
int Write_Wave(char *filename, Sound *snd);

//Soundを作成し、引数の情報に合わせて領域の確保をする。使われる形式以外の領域のポインタはNULL
//成功すればポインタを、失敗すればNULLを返す
Sound *Create_Sound(uint16_t channelnum, uint32_t samplingrate, uint16_t bit_per_sample, uint32_t datasize);

//Soundを開放する
void Free_Sound(Sound *snd);

Sound *Read_Wave(char *filename)
{
  uint32_t i;
  uint8_t header_buf[20];             //フォーマットチャンクのサイズまでのヘッダ情報を取り込む
  FILE *fp;
  Sound *snd;
  uint32_t datasize;                   //波形データのバイト数
  uint16_t fmtid;                     //fmtのIDを格納する
  uint16_t ext_parameter;             //fmtのIDを格納する
  uint16_t channelnum;                //チャンネル数
  uint32_t samplingrate;               //サンプリング周波数
  uint16_t bit_per_sample;            //量子化ビット数
  uint8_t *buf;                       //フォーマットチャンクIDから拡張部分までのデータを取り込む
  uint32_t fmtsize;

  if ((fp = fopen(filename, "rb")) == NULL) {
    fprintf(stderr, "Error: %s could not read.\n", filename);
    return NULL;
  }

  fread(header_buf, sizeof(uint8_t), 20, fp);   //フォーマットチャンクサイズまでのヘッダ部分を取り込む

  //ファイルがRIFF形式であるか
  if (strncmp(header_buf, "RIFF", 4)) {
    fprintf(stderr, "Error: %s is not RIFF.\n", filename);
    fclose(fp);
    return NULL;
  }

  //ファイルがWAVEファイルであるか
  if (strncmp(header_buf + 8, "WAVE", 4)) {
    fprintf(stderr, "Error: %s is not WAVE.\n", filename);
    fclose(fp);
    return NULL;
  }

  //fmt のチェック
  if (strncmp(header_buf + 12, "fmt ", 4)) {
    fprintf(stderr, "Error: %s fmt not found.\n", filename);
    fclose(fp);
    return NULL;
  }

  memcpy(&fmtsize, header_buf + 16, sizeof(fmtsize));
  printf("fmtsize=%d\n",fmtsize);

  if ((buf = (uint8_t *)malloc(sizeof(uint8_t) * fmtsize)) == NULL) {
    fprintf(stderr, "Allocation error size fmtsize %x\n",fmtsize);
    fclose(fp);
    return NULL;
  }

  fread(buf, sizeof(uint8_t), fmtsize, fp);     //フォーマットIDから拡張部分までのヘッダ部分を取り込む

  memcpy(&fmtid, buf, sizeof(fmtid));     //LinearPCMファイルならば1が入る

  if (fmtid != 1) {
    fprintf(stderr, "Error: %s is not LinearPCM.\n", filename);
    fclose(fp);
    return NULL;
  }

  memcpy(&channelnum, buf + 2, sizeof(channelnum)); //チャンネル数を取得
  memcpy(&samplingrate, buf + 4, sizeof(samplingrate)); //サンプリング周波数を取得
  memcpy(&bit_per_sample, buf + 14, sizeof(bit_per_sample)); //量子化ビット数を取得
  if (fmtsize != 16) {
  	memcpy(&ext_parameter, &buf[16], sizeof(ext_parameter));     //拡張パラメータのサイズ
    printf("ext_parameter=%d\n", ext_parameter);
	if (ext_parameter > 0) {
   	  uint8_t dmy[1];
      for(int i=0;i<ext_parameter;i++) {
        fread(dmy, sizeof(uint8_t), 1, fp);     //拡張パラメータを読み捨てる
      }
    }
  }


  while(1) { 
    int len = fread(buf, sizeof(uint8_t), 8, fp);     //chunkのIDとサイズを取得8バイト
    if (len != 8) {
      fprintf(stderr, "Error: %s is unknown format.\n", filename);
      fclose(fp);
      return NULL;
    }
    char id[5] = {0};
    for (int i=0;i<4;i++) {
      id[i] = tolower(buf[i]);
    }
    printf("CHUNK ID=%s\n",id);
    //sleep(10);

    if (strcmp(id, "data") == 0) {
      break;
    } else {
      uint32_t listsize;
      uint8_t dmy;
      memcpy(&listsize, buf + 4, sizeof(listsize));
      printf("listsize=%d\n",listsize);
      for (int i=0;i<listsize;i++) {
        int len = fread(&dmy, sizeof(uint8_t), 1, fp);     //読み捨てる
        if (len != 1) {
          fprintf(stderr, "Error: %s is unknown format.\n", filename);
          fclose(fp);
          return NULL;
        }
      } // end for
    }
  } // end while

  memcpy(&datasize, buf + 4, sizeof(datasize)); //波形データのサイズの取得

  if ((snd = Create_Sound(channelnum, samplingrate, bit_per_sample, datasize)) == NULL) {
    fclose(fp);
    return NULL;
  }

  if (channelnum == 1 && bit_per_sample == 8) {
    fread(snd->monaural8, sizeof(uint8_t), snd->datanum, fp);   //データ部分を全て取り込む
  } else if (channelnum == 1 && bit_per_sample == 16) {
    fread(snd->monaural16, sizeof(int16_t), snd->datanum, fp);
  } else if (channelnum == 2 && bit_per_sample == 8) {
    for (i = 0; i < snd->datanum; i++) {
      fread(&(snd->stereo8[i].l), sizeof(uint8_t), 1, fp);
      fread(&(snd->stereo8[i].r), sizeof(uint8_t), 1, fp);
    }
  } else if (channelnum == 2 && bit_per_sample == 16) {
    for (i = 0; i < snd->datanum; i++) {
      fread(&(snd->stereo16[i].l), sizeof(int16_t), 1, fp);
      fread(&(snd->stereo16[i].r), sizeof(int16_t), 1, fp);
    }
  } else {
    fprintf(stderr, "Header is destroyed.");
    fclose(fp);
    Free_Sound(snd);
  }

  return snd;
}

int Write_Wave(char *filename, Sound *snd)
{
  int i;
  FILE *fp;
  uint8_t header_buf[HEADERSIZE]; //ヘッダを格納する
  uint32_t fswrh;  //リフヘッダ以外のファイルサイズ
  uint32_t fmtchunksize; //fmtチャンクのサイズ
  uint32_t dataspeed;    //データ速度
  uint16_t blocksize;   //1ブロックあたりのバイト数
  uint32_t datasize;     //周波数データのバイト数
  uint16_t fmtid;       //フォーマットID

  if ((fp = fopen(filename, "wb")) == NULL) {
    fprintf(stderr, "Error: %s could not open.\n", filename);
    return 1;
  }

  fmtchunksize = 16;
  blocksize = snd->channelnum * (snd->bit_per_sample / 8);
  dataspeed = snd->samplingrate * blocksize;
  datasize = snd->datanum * blocksize;
  fswrh = datasize + HEADERSIZE - 8;
  fmtid = 1;

  header_buf[0] = 'R';
  header_buf[1] = 'I';
  header_buf[2] = 'F';
  header_buf[3] = 'F';
  memcpy(header_buf + 4, &fswrh, sizeof(fswrh));
  header_buf[8] = 'W';
  header_buf[9] = 'A';
  header_buf[10] = 'V';
  header_buf[11] = 'E';
  header_buf[12] = 'f';
  header_buf[13] = 'm';
  header_buf[14] = 't';
  header_buf[15] = ' ';
  memcpy(header_buf + 16, &fmtchunksize, sizeof(fmtchunksize));
  memcpy(header_buf + 20, &fmtid, sizeof(fmtid));
  memcpy(header_buf + 22, &(snd->channelnum), sizeof(snd->channelnum));
  memcpy(header_buf + 24, &(snd->samplingrate), sizeof(snd->samplingrate));
  memcpy(header_buf + 28, &dataspeed, sizeof(dataspeed));
  memcpy(header_buf + 32, &blocksize, sizeof(blocksize));
  memcpy(header_buf + 34, &(snd->bit_per_sample), sizeof(snd->bit_per_sample));
  header_buf[36] = 'd';
  header_buf[37] = 'a';
  header_buf[38] = 't';
  header_buf[39] = 'a';
  memcpy(header_buf + 40, &datasize, sizeof(datasize));

  fwrite(header_buf, sizeof(uint8_t), HEADERSIZE, fp);

  if (snd->channelnum == 1 && snd->bit_per_sample == 8) {
    fwrite(snd->monaural8, sizeof(uint8_t), snd->datanum, fp);    //データ部分を全て書き込む
  } else if (snd->channelnum == 1 && snd->bit_per_sample == 16) {
    fwrite(snd->monaural16, sizeof(int16_t), snd->datanum, fp);
  } else if (snd->channelnum == 2 && snd->bit_per_sample == 8) {
    for (i = 0; i < snd->datanum; i++) {
      fwrite(&(snd->stereo8[i].l), sizeof(uint8_t), 1, fp);
      fwrite(&(snd->stereo8[i].r), sizeof(uint8_t), 1, fp);
    }
  } else {
    for (i = 0; i < snd->datanum; i++) {
      fwrite(&(snd->stereo16[i].l), sizeof(int16_t), 1, fp);
      fwrite(&(snd->stereo16[i].r), sizeof(int16_t), 1, fp);
    }
  }

  fclose(fp);

  return 0;
}

Sound *Create_Sound(uint16_t channelnum, uint32_t samplingrate, uint16_t bit_per_sample, uint32_t datasize)
{
  Sound *snd;

  if ((snd = (Sound *)malloc(sizeof(Sound))) == NULL) {
    fprintf(stderr, "Allocation error size Sound %lx\n",sizeof(Sound));
    return NULL;
  }

  snd->channelnum = channelnum;
  snd->samplingrate = samplingrate;
  snd->bit_per_sample = bit_per_sample;
  snd->datanum = datasize / (channelnum * (bit_per_sample / 8));
  snd->monaural8 = NULL;
  snd->monaural16 = NULL;
  snd->stereo8 = NULL;
  snd->stereo16 = NULL;

  if (channelnum == 1 && bit_per_sample == 8) {
    if ((snd->monaural8 = (uint8_t *)malloc(datasize)) == NULL) {
      fprintf(stderr, "Allocation error size datasize %x\n",datasize);
      free(snd);
      return NULL;
    }
  } else if (channelnum == 1 && bit_per_sample == 16) {
    if ((snd->monaural16 = (int16_t *)malloc(sizeof(int16_t) * snd->datanum)) == NULL) {
      fprintf(stderr, "Allocation error size sizeof(int16_t) * snd->datanum %lx\n",(long)(sizeof(int16_t) * snd->datanum));
      free(snd);
      return NULL;
    }
  } else if (channelnum == 2 && bit_per_sample == 8) {
    if ((snd->stereo8 = (Soundsample8 *)malloc(sizeof(Soundsample8) * snd->datanum)) == NULL) {
      fprintf(stderr, "Allocation error size sizeof(Soundsample8) * snd->datanum %lx\n",(long)(sizeof(Soundsample8) * snd->datanum));
      free(snd);
      return NULL;
    }
  } else if (channelnum == 2 && bit_per_sample == 16) {
    if ((snd->stereo16 = (Soundsample16 *)malloc(sizeof(Soundsample16) * snd->datanum)) == NULL) {
      fprintf(stderr, "Allocation error size sizeof(Soundsample16) * snd->datanum %lx\n",(long)(sizeof(Soundsample16) * snd->datanum));
      free(snd);
      return NULL;
    }
  } else {
    fprintf(stderr, "Channelnum or Bit/Sample unknown\n");
    free(snd);
    return NULL;
  }

  return snd;
}

void Free_Sound(Sound *snd)
{
  if (snd->channelnum == 1 && snd->bit_per_sample == 8) {
    free(snd->monaural8);
  } else if (snd->channelnum == 1 && snd->bit_per_sample == 16) {
    free(snd->monaural16);
  } else if (snd->channelnum == 2 && snd->bit_per_sample == 8) {
    free(snd->stereo8);
  } else {
    free(snd->stereo16);
  }

  free(snd);
}

int main(int argc, char *argv[])
{
  FILE *fp;
  int i, x, max, min;

  if (argc != 3) {
    fprintf(stderr, "Usage: %s <inputfile> <outputfile>\n", argv[0]);
    exit(1);
  }

  Sound *snd;

  if ((snd = Read_Wave(argv[1])) == NULL) {
    exit(1);
  }

  printf("Sampling rate %d\n", snd->samplingrate);
  printf("bit_per_sample %d\n", snd->bit_per_sample);
  printf("channel number %d\n", snd->channelnum);
  printf("sample number %d\n", snd->datanum);

  fp = fopen(argv[2], "w");
  if (fp == NULL) {
    perror("fopen");
    exit(1);
  }
  max = min = 0x7fff;
  fprintf(fp, "//This file was generated from %s\n", argv[1]);
  fprintf(fp, "#define MUSIC_LEN %d\n", snd->datanum);
  fprintf(fp, "static uint16_t music[] = {\n");
  if (snd->channelnum == 1) {
    for (i = 0; i < snd->datanum; i++) {
      x = snd->monaural16[i] + 0x7fff;
      fprintf(fp, "0x%04x,\n", x);
      if (max < x) max = x;
      if (min > x) min = x;
    }
  } else {
    for (i = 0; i < snd->datanum; i++) {
      x = (snd->stereo16[i].l + snd->stereo16[i].r) / 2 + 0x7fff;
      fprintf(fp, "0x%04x,\n", x);
      if (max < x) max = x;
      if (min > x) min = x;
    }
  }
  fprintf(fp, "};\n");
  Free_Sound(snd);
  printf("data max %d\ndata min %d\n", max, min);
  return 0;
}

