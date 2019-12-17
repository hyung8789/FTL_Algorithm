#include "header.h"

/***
- init, read, write, erase 함수 정의
- 플래시 메모리에 대해 물리적으로 접근하여 작업
***/

int init(FlashMem **flashmem, __int64 megabytes,int mapping_method) //megabytes 크기의 플래시 메모리를 할당
{
	FILE *fp = NULL;
	FILE *volume = NULL;
	FILE *table = NULL;

	char *data_array = NULL; //파일에 기록하기 위한 섹터,블록 또는 MB단위의 배열
	__int64 *mapping_table = NULL; //섹터 매핑 또는 블록 매핑 테이블
	__int64 *block_free_space = NULL; //블록 당 섹터 여유공간을 저장하는 테이블
	
	__int64 initialized_space_size = 0; //매핑 기법에 따른 data_array의 사이즈
	__int64 storage_byte = 0; //매핑 기법에 따른 데이터 저장공간의 총 byte 크기
	__int64 block_size = 0; //할당된 플래시메모리의 전체 블록 개수
	__int64 sector_size = 0; //할당된 플래시메모리의 전체 섹터 개수
	__int64 init_pos_next = 0; //파일 포인터의 다음 위치

	//이미 플래시 메모리를 생성하였을 경우 다시 이니셜라이징시 덮어씌움
	*flashmem = new FlashMem(megabytes);

	storage_byte = (*flashmem)->calc_storage_byte(); //데이터 저장 공간의 총 byte 크기
	block_size = (*flashmem)->calc_block_size(); //전체 블록 개수
	sector_size = (*flashmem)->calc_sector_size(); //전체 섹터 개수

	data_array = new char[MB_PER_BYTE]; //1MB 크기의 저장공간 생성
	memset(data_array, NULL, MB_PER_BYTE); //NULL값으로 모두 초기화(바이트 단위)

	switch (mapping_method) //매핑 방식에 따라 매핑 테이블 크기 설정
	{
	case 0: //섹터 매핑 기법
		//블록단위로 해당 블록의 여유공간을 기록
		block_free_space = new __int64[block_size];
		for (__int64 i = 0; i < block_size; i++) //0~31의 섹터 개수, BLOCK_PER_SECTOR로 전체 초기화
		{
			block_free_space[i] = BLOCK_PER_SECTOR;
		}
		 
	
		//섹터 단위 매핑 테이블(LSN -> PSN) 생성
		mapping_table = new __int64[sector_size]; //섹터 개수 크기로 생성
		for (__int64 i = 0; i < sector_size; i++)
		{
			mapping_table[i] = MAPPING_INIT_VALUE;
		}

		initialized_space_size = MB_PER_BYTE; //매핑 기법에 따른 data_array의 사이즈
	
		break;

	case 1: //블록 매핑 기법
		//블록단위로 해당 블록의 여유공간을 기록
		block_free_space = new __int64[block_size];
		for (__int64 i = 0; i < block_size; i++)
		{
			block_free_space[i] = BLOCK_PER_SECTOR;
		}

		//블록 단위 매핑 테이블(LBN -> PBN) 생성
		mapping_table = new __int64[block_size]; //블록 개수 크기로 생성
		for (__int64 i = 0; i < block_size; i++)
		{
			mapping_table[i] = MAPPING_INIT_VALUE;
		}

		initialized_space_size = MB_PER_BYTE; //매핑 기법에 따른 data_array의 사이즈
		
		break;

	default: //매핑 기법 사용안함
		initialized_space_size = MB_PER_BYTE; //매핑 기법에 따른 data_array의 사이즈
		break;
	}
	
	//플래시 메모리 스토리지 파일 생성
	if ((fp = fopen("storage.bin", "wb")) == NULL) //쓰기 + 이진파일 모드
	{
		fprintf(stderr, "storage.bin 파일을 쓰기모드로 열 수 없습니다.");
		delete[] data_array; //메모리상에서 제거
		return 0;
	}

	//입력받은 MB만큼 파일에 기록
	init_pos_next = _ftelli64(fp);
	while (1)
	{
		fwrite(data_array, sizeof(char), initialized_space_size, fp); //데이터 저장 공간 기록
		init_pos_next += initialized_space_size;
		if (init_pos_next >= storage_byte) break; //다음에 기록할 위치가 바이트단위로 환산한 저장공간의 용량을 넘을 경우 종료
	}

	delete[] data_array; //메모리상에서 data_array 제거

	//플래시 메모리 용량 및 매핑 방법 기록
	if ((volume = fopen("volume.txt", "wt")) == NULL) //쓰기 + 텍스트파일 모드
	{
		fprintf(stderr, "volume.txt 파일을 쓰기모드로 열 수 없습니다.");
		if (fp != NULL)	fclose(fp);
		return 0;
	}
	
	//플래시 메모리 용량 및 매핑 방법 기록
	fprintf(volume, "%I64d\n", (*flashmem)->get_flashmem_size());
	fprintf(volume, "%d", mapping_method);

	//블록 단위의 여유공간 관리 테이블, 매핑 테이블 기록
	if ((table = fopen("table.bin", "wb")) == NULL) //쓰기 + 이진파일 모드
	{
		fprintf(stderr, "table.bin 파일을 쓰기모드로 열 수 없습니다.");
		if (volume != NULL)	fclose(volume);
		return 0;
	}

	switch (mapping_method) //블록 단위의 여유공간 관리 테이블, 매핑 테이블 기록
	{
	case 0: //섹터 매핑 방식
		fwrite(block_free_space, sizeof(__int64), block_size, table); //블록 단위의 여유공간 관리 테이블 기록
		fwrite(mapping_table, sizeof(__int64), sector_size, table); //매핑 테이블 기록
		if ((*flashmem)->block_free_space != NULL && (*flashmem)->mapping_table != NULL) //다시 init할 경우 초기화 및 새로 연결
		{
			//메모리 해제
			delete[] (*flashmem)->block_free_space;
			delete[] (*flashmem)->mapping_table;

			//링크 초기화
			(*flashmem)->block_free_space = NULL;
			(*flashmem)->mapping_table = NULL;
		}

		(*flashmem)->block_free_space = block_free_space; //블록 단위의 여유공간 관리 테이블 연결
		(*flashmem)->mapping_table = mapping_table; //매핑 테이블 연결
		break;
	case 1: //블록 매핑 방식
		fwrite(block_free_space, sizeof(__int64), block_size, table); //블록 단위의 여유공간 관리 테이블 기록
		fwrite(mapping_table, sizeof(__int64), block_size, table); //매핑 테이블 기록
		if ((*flashmem)->block_free_space != NULL && (*flashmem)->mapping_table != NULL) //다시 init할 경우 초기화 및 새로 연결
		{
			//메모리 해제
			delete[] (*flashmem)->block_free_space;
			delete[] (*flashmem)->mapping_table;

			//링크 초기화
			(*flashmem)->block_free_space = NULL;
			(*flashmem)->mapping_table = NULL;
		}
		(*flashmem)->block_free_space = block_free_space; //블록 단위의 여유공간 관리 테이블 연결
		(*flashmem)->mapping_table = mapping_table; //매핑 테이블 연결
		break;

	default:
		break;
	}

	fclose(fp);
	fclose(volume);
	fclose(table);

	std::cout << megabytes << " megabytes flash memory" << std::endl;

	return 1;
}

char Flash_read(FlashMem **flashmem, __int64 PSN, int mapping_method) //물리 섹터에 데이터를 읽어옴
{
	FILE *fp = NULL;

	__int64 PBN = 0; //PSN이 위치하고 있는 물리 블록 번호
	__int64 read_pos = 0; //읽고자 하는 물리 섹터의 위치
	char buffer=NULL; //읽어들인 데이터

	if (*flashmem == NULL) //플래시 메모리가 할당되지 않았을 경우
	{
		std::cout << "not initialized" << std::endl;
		return 0;
	}

	if (PSN > (MB_PER_SECTOR * (*flashmem)->get_flashmem_size()) - 1) //범위 초과 오류
	{
		std::cout << "out of range error" << std::endl;
		return 0;
	}

	if ((fp = fopen("storage.bin", "rb")) == NULL) //읽기 + 이진파일 모드
	{
		fprintf(stderr, "storage.bin 파일을 읽기모드로 열 수 없습니다.");
		return 0;
	}

	read_pos = BYTE_PER_SECTOR * PSN; //읽고자 하는 물리 섹터의 위치

	_fseeki64(fp, read_pos, SEEK_SET); //읽고자 하는 물리 섹터의 위치로 이동
	fread(&buffer, sizeof(char), 1, fp); //해당 물리 섹터에 기록된 값 읽기
	fclose(fp);

	if (buffer == NULL)	return '\0';
	else return buffer;
}

int Flash_write(FlashMem **flashmem, __int64 PSN, char data, int mapping_method) //물리 섹터에 데이터를 기록
{
	char buffer=NULL;
	__int64 PBN = 0; //PSN이 위치하고 있는 물리 블록 번호
	__int64 write_pos = 0; //쓰고자 하는 위치

	//이미 입력된 위치에 데이터 입력 시도시 overwrite 오류 발생
	if (*flashmem == NULL) //플래시 메모리가 할당되지 않았을 경우
	{
		std::cout << "not initialized" << std::endl;
		return 0;
	}

	if (PSN > (MB_PER_SECTOR * (*flashmem)->get_flashmem_size()) - 1) //범위 초과 오류
	{
		std::cout << "out of range error" << std::endl;
		return 0;
	}

	FILE *fp = NULL;
	if ((fp = fopen("storage.bin", "rb+")) == NULL) //읽고 쓰기 모드 + 이진파일 모드
	{
		fprintf(stderr, "storage.bin 파일을 읽고 쓰기 모드로 열 수 없습니다.");
		return 0;
	}

	write_pos = BYTE_PER_SECTOR * PSN; //쓰고자 하는 위치


	_fseeki64(fp, write_pos, SEEK_SET); //읽고자 하는 물리 섹터의 위치로 이동
	fread(&buffer, sizeof(char), 1, fp); //해당 물리 섹터에 기록된 값 읽기
	_fseeki64(fp, write_pos, SEEK_SET); //쓰고자 하는 물리 섹터의 위치로 이동

	if (buffer != NULL) 
	{
		std::cout << "overwrite error" << std::endl;
		fclose(fp);
		return 0;
	}
	else
	{
		fwrite(&data, sizeof(char), 1, fp); //데이터 기록
		std::cout << "done" << std::endl;
	}

	fclose(fp);
	return 1;
}

int Flash_erase(FlashMem **flashmem, __int64 PBN, int mapping_method) //물리 블록에 해당하는 데이터를 지움
{
	FILE *fp = NULL;

	char erase_buffer = NULL; //지우고자 할 때 덮어씌우고자 할 값
	__int64 erase_pos_start = 0; //지우고자 하는 블록 위치의 시작 
	__int64 erase_pos_end = 0; //지우고자 하는 블록 위치의 끝
	__int64 erase_pos_next = 0; //erase할 다음 섹터의 위치
	int erase_size = 0; //매핑 방식에 따른 erase 연산 시 파일 포인터 이동 사이즈

	//해당 블록이 속한 섹터들에 대해서 모두 erase

	if (*flashmem == NULL) //플래시 메모리가 할당되지 않았을 경우
	{
		std::cout << "not initialized" << std::endl;
		return 0;
	}

	if (PBN > (MB_PER_BLOCK * (*flashmem)->get_flashmem_size()) - 1) //범위 초과 오류
	{
		std::cout << "out of range error" << std::endl;
		return 0;
	}

	if ((fp = fopen("storage.bin", "rb+")) == NULL) //읽고 쓰기모드 + 이진파일 모드(쓰고 읽기 모드로 열 경우 파일내용이 모두 초기화)
	{
		fprintf(stderr, "storage.bin 파일을 읽고 쓰기 모드로 열 수 없습니다.");
		return 0;
	}


	erase_pos_start = BYTE_PER_BLOCK * PBN; //지우고자 하는 블록 위치의 시작 
	erase_pos_end = (BYTE_PER_BLOCK * (PBN + 1)) - 1; //지우고자 하는 블록 위치의 끝
	erase_size = BYTE_PER_SECTOR; //매핑 방식에 따른 erase 연산 시 파일 포인터 이동 사이즈

	_fseeki64(fp, erase_pos_start, SEEK_SET); //erase하고자 하는 물리 블록의 시작 위치로 이동
	erase_pos_next = _ftelli64(fp); //erase할 다음 섹터의 위치
	while(1) //해당 블록 위치의 끝까지 반복
	{
		fwrite(&erase_buffer, sizeof(char), 1, fp);
		erase_pos_next += erase_size;
		if ((erase_pos_next > erase_pos_end)) break;
		else _fseeki64(fp, erase_pos_next, SEEK_SET); //한 섹터에 대한 바이트 단위(512byte)로 이동
	}

	//섹터 매핑 또는 블록 매핑 시 Erase 연산 수행시 해당 블록의 블록 여유 공간 테이블값도 초기화해야함
	if (mapping_method == 0 || mapping_method == 1)
		(*flashmem)->block_free_space[PBN] = MAPPING_INIT_VALUE;

	fclose(fp);
	std::cout << PBN << "-th block erased" << std::endl;

	return 1;
}