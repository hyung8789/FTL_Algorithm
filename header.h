#define _CRT_SECURE_NO_WARNINGS //_CRT_SECURE_NO_WARNINGS
#include <stdio.h> //fscanf,fprinf,fwrite,fread
#include <iostream> //입출력
#include <sstream> //stringstream
#include <windows.h> //시스템 명령어
#define MB_PER_BLOCK 64 //1MB당 64 블록
#define MB_PER_SECTOR 2048 //1MB당 2048섹터
#define MB_PER_BYTE 1048576 //1MB당 바이트 환산값(MB_PER_SECTOR * BYTE_PER_SECTOR / MB_PER_BLOCK)
#define BLOCK_PER_SECTOR 32 //한 블록에 해당하는 섹터의 개수
#define BYTE_PER_BLOCK 16384 //한 블록에 해당하는 바이트의 크기(BYTE_PER_SECTOR * BLOCK_PER_SECTOR)
#define BYTE_PER_SECTOR 512 //한 섹터에 해당하는 바이트의 크기
#define MAPPING_INIT_VALUE -1 //매핑 테이블의 초기값

extern int mapping_method; //현재 플래시 메모리의 매핑방식 (default : 2 - 사용하지 않음)

// x64로 컴파일하여야 size_t가 unsigned __int64로 인식
// 플래시 메모리의 할당된 전체 용량에 대한 Free Space는 관리하도록 하지 않았으므로 더 이상 기록할 공간이 없을 경우 오류발생 할 수 있음

/***
1섹터 = 512byte
1블록 = 32섹터 = 32*512 = 16,384byte
1MB = 1024KB
1KB = 1024byte
1MB의 섹터 수 = (1*1024*1024)/512(1섹터당 byte) = 2048개
1MB의 블록 수(32섹터) = 2048/32 = 64개
--
unsigned int arrange == unsigned long arrange : 0 ~ 4,294,967,295
4 Gigabyte를 byte 단위로 변환하면 4,294,967,296byte이므로 자료형의 범위를 넘어섬
따라서, 더 큰 용량의 플래시 메모리를 생성하기 위해서는
unsigned __int64를 사용하여야 하는데 파일 포인터 _fseeki64가 __int64(long long)까지만 지원하므로 __int64를 사용
--
unsigned __int64 arrange: 0 ~ 18,446,744,073,709,551,615
__int64 arrange : ?9,223,372,036,854,775,808 ~ 9,223,372,036,854,775,807
***/

class FlashMem {
public:
	FlashMem();
	FlashMem(__int64 megabytes); //megabytes 크기의 플래시 메모리 생성

	__int64 *block_free_space; //블록 단위 여유공간 관리 테이블
	__int64 *mapping_table; //매핑 테이블

	void disp_command(int mapping_method); //커맨드 명령어 출력
	void input_command(FlashMem **flashmem, int &mapping_method); //커맨드 명령어 입력

	void chk_initialized(FlashMem **flashmem, int &mapping_method); //플래시 메모리가 이미 생성되어 있는지 확인
	void disp_flash_info(FlashMem **flashmem, int mapping_method); //현재 생성된 플래시 메모리의 정보 출력
	void load_table(FlashMem **flashmem, int mapping_method); //매핑 방식에 따른 매핑 테이블 또는 블록 단위 여유공간 테이블 로드
	void save_table(FlashMem **flashmem, int mapping_method); //매핑 방식에 따른 매핑 테이블 또는 블록 단위 여유공간 테이블 저장
	int switch_mapping_method(); //현재 플래시 메모리의 매핑 방식 변경
	
	__int64 get_flashmem_size(); //플래시 메모리 크기 반환
	__int64 calc_block_size(); //할당된 메모리 크기에 해당하는 블록의 개수 반환
	__int64 calc_sector_size(); //할당된 메모리 크기에 해당하는 섹터의 개수 반환
	__int64 calc_storage_byte(); //할당된 메모리 크기에 해당하는 총 byte 크기를 반환

private:
	__int64 flashmem_size; //플래시 메모리의 MB크기
};

int init(FlashMem **flashmem, __int64 megabytes, int mapping_method); //megabytes 크기의 플래시 메모리를 할당
char Flash_read(FlashMem **flashmem, __int64 PSN, int mapping_method); //물리 섹터에 데이터를 읽어옴
int Flash_write(FlashMem **flashmem, __int64 PSN, char data, int mapping_method); //물리 섹터에 데이터를 기록
int Flash_erase(FlashMem **flashmem, __int64 PBN, int mapping_method); //물리 블록에 해당하는 데이터를 지움

//FTL Algorithm
void Print_table(FlashMem **flashmem, int mapping_method);  //매핑 테이블 출력
int FTL_read(FlashMem **flashmem, __int64 LSN, int mapping_method); //논리 섹터 또는 논리 블록에 해당되는 매핑테이블 상 물리 섹터 또는 물리 블록의 위치를 반환
int FTL_write(FlashMem **flashmem, __int64 LSN, char data, int mapping_method); //논리 섹터 또는 논리 블록에 해당되는 매핑테이블 상 물리 섹터 또는 물리 블록 위치에 기록