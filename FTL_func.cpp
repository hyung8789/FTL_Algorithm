#include "header.h"

/***
- Print_table, FTL_read, FTL_write 정의
- 논리 섹터 번호 또는 논리 블록 번호를 매핑테이블과 대조하여 physical_func상의 read, write, erase에 전달하여 작업을 수행
***/

/*************************************************
mapping_method 0 = 섹터 매핑 방식
mapping_method 1 = 블록 매핑 방식
---

< 섹터 매핑 >
- 블록 단위 크기의 블록의 Free Space(여유 공간) 테이블 생성 (초기값 : 32 (블록 당 섹터개수), 여유 공간 없음 : 0)
- 추가 파일을 만들어서 블록 단위의 Free Space 테이블 및 LSN->PSN 변환 매핑 테이블 관리
- 데이터 기록 시 물리 섹터에 0번부터 순차적으로 기록, 매핑 테이블 상에서 LSN에 해당 하는 PSN값 변경 및 해당 블록의 Free Space(여유 공간) 값 감소
- 데이터를 같은 논리 섹터 위치에 기록 시 다른 물리 섹터에 기록 및 매핑 테이블 상에서 LSN에 해당 하는 PSN값 변경
- 해당 블록이 가득 찼을 경우 매핑 테이블 상에서 대응되지 않는 값들을 제외한 나머지를 다른 빈 블록에 복사,해당 블록 Erase,
옮겨진 블록의 Free Space 테이블 값 변경, 매핑 테이블 상에서 LSN에 해당 하는 PSN값 변경(Trim)

< 블록 매핑 >
- 추가 파일을 만들어서 블록 단위의 Free Space 테이블, 블록 단위 매핑 테이블(LBN -> PBN) 관리
- 데이터 기록 시
1) 사용자가 입력한 논리 섹터 번호(LSN)을 바탕으로 논리 블록 번호(LBN)를 LSN / 블록 당 섹터 개수(32)로 구하기
2) 블록 단위의 매핑 테이블 내에서 물리 블록 번호(PBN)를 얻기(물리 블록 0번부터 순차적으로 할당)
3) 물리 블록 내에서 섹터 Offset(0~31)을 LSN % 블록 당 섹터 개수(32)로 알아내기
4) 해당 물리 섹터(PSN)에 기록

- 같은 위치에 기록 시도 시
1) 해당 물리 블록내의 기록을 시도하는 위치의 데이터를 제외한 모든 데이터를 빈 다른 블록에 복사 후 해당 블록 Erase
2) 새로운 데이터 기록

- 모든 데이터들로 가득 찬 블록에 다시 입력 시도 할 경우(즉, 사용자가 입력한 LSN 위치에 이미 데이터가 입력되어있고, 해당 LSN이 속한 물리 블록의 값들이 전부 데이터가 입력된 섹터들로 채워져있을 경우)
해당 블록에 대해서는 더 이상 기록 불가
************************************************/

void Print_table(FlashMem **flashmem, int mapping_method) //매핑 테이블 출력
{
	__int64 table_size = 0; //매핑 테이블의 크기
	__int64 table_index = 0; //매핑 테이블 인덱스

	if (*flashmem == NULL) //플래시 메모리가 할당되지 않았을 경우
	{
		std::cout << "not initialized" << std::endl;
		return;
	}

	//블록 단위 여유 공간 관리 테이블, 매핑 테이블이 로드되지 않았을 시 파일로부터 메모리상에 로드
	if ((*flashmem)->mapping_table == NULL && (*flashmem)->block_free_space == NULL)
		(*flashmem)->load_table(flashmem, mapping_method);

	switch (mapping_method) //매핑 방식에 따라 매핑 테이블 출력 설정
	{
	case 0: //섹터 매핑 방식
		table_size=(*flashmem)->calc_sector_size(); //섹터 수
		std::cout<<"< Sector Mapping Table (LSN -> PSN) >"<<std::endl;
		break;

	case 1: //블록 매핑 방식
		table_size=(*flashmem)->calc_block_size(); //블록 수
		std::cout<<"< Block Mapping Table (LBN -> PBN) >"<<std::endl;
		break;

	default:
		return;
	}

	while(table_index < table_size)
	{
		//할당된 값들만 출력
		if((*flashmem)->mapping_table[table_index] == MAPPING_INIT_VALUE)
		{
			table_index++;
		}
		else
		{
			std::cout << table_index << " -> " << (*flashmem)->mapping_table[table_index] << std::endl;
			table_index++;
		}
	}
}

int FTL_read(FlashMem **flashmem, __int64 LSN, int mapping_method) //논리 섹터 또는 논리 블록에 해당되는 매핑테이블 상 물리 섹터 또는 물리 블록의 위치를 읽어옴
{

	__int64 LBN = 0; //해당 논리 섹터가 위치하고 있는 논리 블록
	__int64 PBN = 0; //해당 물리 섹터가 위치하고 있는 물리 블록
	__int64 PSN = 0; //실제로 저장된 물리 섹터 번호
	int physical_sector_offset = 0; //블록 내의 섹터 offset = 0 ~ 31
	char data_output = NULL; //물리 섹터로부터 읽어들인 데이터

	if (*flashmem == NULL) //플래시 메모리가 할당되지 않았을 경우
	{
		std::cout << "not initialized" << std::endl;
		return 0;
	}

	if (LSN > (MB_PER_SECTOR * (*flashmem)->get_flashmem_size()) - 1) //범위 초과 오류
	{
		std::cout << "out of range error" << std::endl;
		return 0;
	}

	//블록 단위 여유 공간 관리 테이블, 매핑 테이블이 로드되지 않았을 시 파일로부터 메모리상에 로드
	if ((*flashmem)->mapping_table == NULL && (*flashmem)->block_free_space == NULL)
		(*flashmem)->load_table(flashmem, mapping_method);

	switch (mapping_method) //매핑 방식에 따라 물리 섹터에 접근
	{
	case 0: //섹터 매핑 기법 (LSN -> PSN)
		PSN = (*flashmem)->mapping_table[LSN];
		//LSN에 대응되는 PSN이 없을 경우
		if (PSN == MAPPING_INIT_VALUE)
		{
			std::cout << "no data" << std::endl;
			return 0;
			
		}
		else //해당되는 물리 섹터의 데이터를 읽기
		{
			std::cout << "PSN : " << PSN <<std::endl; //물리 섹터 번호 출력
			data_output = Flash_read(flashmem, PSN, mapping_method); //데이터 읽기
			std::cout << data_output << std::endl;
		}
		return 1;

	case 1: //블록 매핑 기법 (LSN -> LBN -> PBN -> PSN)
		//사용자가 입력한 LSN으로 LBN을 구하고 대응되는 PBN과 물리 섹터 번호를 구함
		LBN = LSN / BLOCK_PER_SECTOR; //해당 논리 섹터가 위치하고 있는 논리 블록
		PBN = (*flashmem)->mapping_table[LBN];
		physical_sector_offset = LSN % BLOCK_PER_SECTOR; //블록 내의 섹터 offset = 0 ~ 31
		PSN = ((*flashmem)->mapping_table[LBN] * BLOCK_PER_SECTOR) + physical_sector_offset; //실제로 저장된 물리 섹터 번호

		//LBN에 대응되는 PBN이 없을 경우
		if (PBN == MAPPING_INIT_VALUE)
		{
			std::cout << "no data" << std::endl;
			return 0;
		}
		else //해당되는 물리 블록의 물리 섹터 출력
		{
			
			data_output = Flash_read(flashmem, PSN, mapping_method); //데이터 읽기
			if (data_output != NULL)
			{
				std::cout << "PSN : " << PSN << std::endl; //물리 섹터 번호 출력
				std::cout << data_output << std::endl;
			}
			else
			{
				std::cout << "PSN : " << PSN << std::endl; //물리 섹터 번호 출력
				std::cout << "no data" << std::endl;
			}
		}
		return 1;

	default:
		return 0;
	}
	return 0;
}

int FTL_write(FlashMem **flashmem, __int64 LSN, char data, int mapping_method) //논리 섹터 또는 논리 블록에 해당되는 매핑테이블 상 물리 섹터 또는 물리 블록 위치에 기록
{
	int flash_write_count = 0; //실제로 수행된 Flash_write 횟수 카운터
	int flash_erase_count = 0; //실제로 수행된 Flash_erase 횟수 카운터
	__int64 flash_erase_block_num = -1; //실제로 삭제된 물리 블록 번호

	char read_buffer[BLOCK_PER_SECTOR] = {NULL}; //한 블록내의 데이터 임시 저장 변수
	int read_buffer_index = 0; //데이터를 읽어서 임시저장하기 위한 read_buffer 인덱스 변수
	int physical_sector_offset = 0; //블록 내의 섹터 offset = 0 ~ 31
	__int64 next_block = 0; //현재 블록이 가득 찰 경우 데이터를 복사할 다른 블록의 위치
	__int64 PBN = 0; //해당 물리 섹터가 위치하고 있는 물리 블록
	__int64 LBN = 0; //해당 논리 섹터가 위치하고 있는 논리 블록
	__int64 PSN = 0; //실제로 저장된 물리 섹터 번호

	if (*flashmem == NULL) //플래시 메모리가 할당되지 않았을 경우
	{
		std::cout << "not initialized" << std::endl;
		return 0;
	}

	if (LSN > (MB_PER_SECTOR * (*flashmem)->get_flashmem_size()) - 1) //범위 초과 오류
	{
		std::cout << "out of range error" << std::endl;
		return 0;
	}

	//블록 단위 여유 공간 관리 테이블, 매핑 테이블이 로드되지 않았을 시 파일로부터 메모리상에 로드
	if ((*flashmem)->mapping_table == NULL && (*flashmem)->block_free_space == NULL)
		(*flashmem)->load_table(flashmem, mapping_method);

	switch (mapping_method)
	{
	case 0: //섹터 매핑 방식

		/*** 최초 매핑 테이블에서 할당 시 순차적으로 할당 ***/
		if ((*flashmem)->mapping_table[LSN] == MAPPING_INIT_VALUE)
		{
			for (__int64 i = 0; i < (*flashmem)->calc_block_size(); i++) //꽉 차지 않은 블록을 찾을때까지 반복
			{
				if ((*flashmem)->block_free_space[i] > 0)
				{
					PBN = (__int64)i;
					break;
				}
			}
			/*** 해당 블록 내에서 빈 섹터를 찾을 때 까지 반복 ***/
			PSN = PBN * BLOCK_PER_SECTOR;
			for (int j = 0; j < BLOCK_PER_SECTOR; j++)
			{
				if (Flash_read(flashmem, PSN, mapping_method) == NULL) //데이터를 반환하지 않을 시 빈 공간
				{
					(*flashmem)->mapping_table[LSN] = PSN; //해당 위치로 설정
					break;
				}
				PSN++;
			}
		}
		
		PSN = (*flashmem)->mapping_table[LSN]; //매핑 테이블에서 논리 섹터 번호에 해당 되는 물리 섹터 번호
		PBN = PSN / BLOCK_PER_SECTOR; //해당 물리 섹터가 위치하고 있는 물리 블록
		LBN = LSN / BLOCK_PER_SECTOR; //해당 논리 섹터가 위치하고 있는 논리 블록

		if ((*flashmem)->block_free_space[PBN] == 0) //해당 물리 블록이 가득 찼을 경우
		{
			/***
			매핑 테이블 상에서 대응되지 않는 값들을 제외한 나머지를 다른 빈 블록에 복사
			매핑 테이블 상에서 LSN에 해당 하는 PSN값 변경(Trim)
			***/

			__int64 LSN_trim_start = LBN * BLOCK_PER_SECTOR; //정렬하고자 하는 매핑 테이블 상 논리 섹터의 시작 위치
			__int64 LSN_trim_end = LSN_trim_start + BLOCK_PER_SECTOR; //정렬하고자 하는 매핑 테이블 상 논리 섹터의 끝 위치
			
			read_buffer_index = 0; //인덱스 초기화
			for (__int64 i = LSN_trim_start; i < LSN_trim_end; i++)
			{
				if ((*flashmem)->mapping_table[i] != MAPPING_INIT_VALUE) //매핑 테이블 상에서 대응되는 데이터들만 저장하므로 read_buffer에 순차적으로 정렬
				{
					(*flashmem)->mapping_table[i] = MAPPING_INIT_VALUE;
					read_buffer[read_buffer_index] = Flash_read(flashmem, (*flashmem)->mapping_table[i], mapping_method);
					read_buffer_index++; //read_buffer에 대한 인덱스 값 증가
				}
			}

			/*** 해당 블록을 Erase ***/
			if (Flash_erase(flashmem, PBN, mapping_method) != 0)
			{
				flash_erase_count++; //erase 카운트 증가
				flash_erase_block_num = PBN;
			}

			/*** 빈 블록 찾기 ***/
			for (__int64 i = 0; i < (*flashmem)->calc_block_size(); i++) //빈 블록을 찾을때까지 반복
			{
				if ((*flashmem)->block_free_space[i] == BLOCK_PER_SECTOR)
				{
					next_block = i;
					break;
				}
			}

			/*** 매핑 테이블에서 값 변경 ***/
			__int64 PSN_copy_start = next_block * BLOCK_PER_SECTOR; //값을 옮기고자 하는 물리 섹터의 시작 위치
			__int64 PSN_copy_end = PSN_copy_start + BLOCK_PER_SECTOR; //값을 옮기고자 하는 물리 섹터의 끝 위치
			__int64 PSN_copy_start_index = PSN_copy_start; //PSN_copy_start 값을 보존하고 인덱싱

			read_buffer_index = 0; //인덱스 초기화
			for (__int64 i = LSN_trim_start; i < LSN_trim_end; i++) //매핑 테이블 상 논리 섹터 번호에 대해 각각 물리 섹터 번호를 할당
			{
				if (read_buffer[read_buffer_index] != NULL) //read_buffer에 기록된 수(옮기고자 하는 수만큼)만큼 매핑 테이블 변경
				{
					(*flashmem)->mapping_table[i] = PSN_copy_start_index;
					PSN_copy_start_index++;
					read_buffer_index++;
				}
				else break; //read_buffer가 처음으로 NULL인 순간 더 이상 기록할 값이 없음
			}


			/*** 빈 블록으로 값들을 복사 ***/
			read_buffer_index = 0; //인덱스 초기화
			for (__int64 i = PSN_copy_start; i < PSN_copy_end; i++)
			{
				if (read_buffer[read_buffer_index] != NULL)
				{
					Flash_write(flashmem, i, read_buffer[read_buffer_index], mapping_method);
					flash_write_count++; //쓰기 카운트 증가
					(*flashmem)->block_free_space[(*flashmem)->mapping_table[next_block]] -= 1;//블록 단위 여유 공간 테이블 감소
					read_buffer_index++;
				}
				else break; //read_buffer가 처음으로 NULL인 순간 더 이상 기록할 값이 없음
			}
		}

		if (Flash_write(flashmem, PSN, data, mapping_method) != 0)
		{
			flash_write_count++; //성공적으로 기록 할 시 카운트 증가
			(*flashmem)->block_free_space[PBN] -= 1;//블록 단위 여유 공간 테이블 감소
		}
		else //같은 위치에 기록 시도 시 다른 섹터에 기록 및 매핑 테이블 상에서 LSN에 해당 하는 PSN값 변경
		{
			/*** 해당 블록 내에서 빈 섹터를 찾을 때 까지 반복 ***/
			PSN = PBN * BLOCK_PER_SECTOR;
			for (int i = 0; i < BLOCK_PER_SECTOR; i++)
			{
				if (Flash_read(flashmem, PSN, mapping_method) == NULL) //NULL 반환 시 빈 공간
				{
					(*flashmem)->mapping_table[LSN] = PSN; //해당 위치로 설정
					break;
				}
				PSN++;
			}

			if (Flash_write(flashmem, PSN, data, mapping_method) != 0)
			{
				flash_write_count++; //성공적으로 기록 할 시 카운트 증가
				(*flashmem)->block_free_space[PBN] -= 1;//블록 단위 여유 공간 테이블 감소
			}
		}

		break;

	case 1: //블록 매핑 방식
		//사용자가 입력한 LSN으로 LBN을 구하고 대응되는 PBN과 물리 섹터 번호를 구함
		LBN = LSN / BLOCK_PER_SECTOR; //해당 논리 섹터가 위치하고 있는 논리 블록
		physical_sector_offset = LSN % BLOCK_PER_SECTOR; //블록 내의 섹터 offset = 0 ~ 31

		/*** 최초 매핑 테이블에서 할당 시 순차적으로 할당 ***/
		if ((*flashmem)->mapping_table[LBN] == MAPPING_INIT_VALUE)
		{
			for (__int64 i = 0; i < (*flashmem)->calc_block_size(); i++) //빈 블록을 찾을때까지 반복
			{
				if ((*flashmem)->block_free_space[i] == BLOCK_PER_SECTOR)
				{
					(*flashmem)->mapping_table[LBN] = i;
					break;
				}
			}
		}

		PBN = (*flashmem)->mapping_table[LBN]; //실제로 저장된 물리 블록 번호
		PSN = (PBN * BLOCK_PER_SECTOR) + physical_sector_offset; //실제로 저장된 물리 섹터 번호

		if ((*flashmem)->block_free_space[PBN] == 0) //해당 블록에 여유공간이 없을 경우
		{
			std::cout << "해당 물리 블록에 더 이상 기록 할 수 없습니다." << std::endl;
			return 0;
		}

		if (Flash_write(flashmem, PSN, data, mapping_method) != 0)
		{
			flash_write_count++; //성공적으로 기록 할 시 카운트 증가
			(*flashmem)->block_free_space[PBN] -= 1;//블록 단위 여유 공간 테이블 감소
		}
		else //이미 데이터가 기록되어 있는 경우
		{
			//해당 물리 블록내의 기록을 시도하는 위치의 데이터를 제외한 모든 데이터를 빈 다른 블록에 복사 후 해당 블록 Erase

			for (__int64 i = 0; i < (*flashmem)->calc_block_size(); i++) //빈 블록을 찾을때까지 반복
			{
				if ((*flashmem)->block_free_space[i] == BLOCK_PER_SECTOR)
				{
					next_block = i;
					break;
				}
			}

			/*** 해당 블록의 모든 내용을 읽어 임시저장 ***/

			//섹터 offset을 일치시켜야 하므로 정렬시키지않고 값들을 읽어야함
			__int64 PSN_trim_pos = (*flashmem)->mapping_table[LBN] * BLOCK_PER_SECTOR; //정렬을 시작할 물리 섹터 위치
			__int64 PSN_exclude_pos = (*flashmem)->mapping_table[LBN] * BLOCK_PER_SECTOR + physical_sector_offset;//제외할 섹터 (새로운 값을 기록할 위치)
			read_buffer_index = 0; //인덱스 초기화
			for (int i = 0; i < BLOCK_PER_SECTOR; i++)
			{
				if (PSN_trim_pos == PSN_exclude_pos) //새로운 값을 기록할 섹터 제외
				{
					read_buffer[read_buffer_index] = NULL;
					read_buffer_index++;
					PSN_trim_pos++;
				}
				else
				{
					read_buffer[read_buffer_index] = Flash_read(flashmem, PSN_trim_pos, mapping_method);
					read_buffer_index++;
					PSN_trim_pos++;
				}
			}

			/*** 해당 블록을 Erase ***/
			if (Flash_erase(flashmem, (*flashmem)->mapping_table[LBN], mapping_method) != 0)
			{
				flash_erase_count++; //erase 카운트 증가
				flash_erase_block_num = (*flashmem)->mapping_table[LBN];
			}

			/*** 매핑 테이블에서 값 변경 ***/
			(*flashmem)->mapping_table[LBN] = next_block;
			PBN = (*flashmem)->mapping_table[LBN]; //실제로 저장된 물리 블록 번호

			/*** 빈 블록에 복사 ***/
			__int64 copy_start_pos = next_block * BLOCK_PER_SECTOR;

			for (int i = 0; i < BLOCK_PER_SECTOR; i++)
			{
				if (read_buffer[i] != NULL) //물리 섹터로부터 읽어들인만큼 기록
				{
					if (Flash_write(flashmem, copy_start_pos, read_buffer[i], mapping_method) != 0)
					{
						flash_write_count++; //쓰기 카운트 증가
						copy_start_pos++;
						(*flashmem)->block_free_space[PBN] -= 1;//블록 단위 여유 공간 테이블 감소
					}
				}
				else copy_start_pos++;
			}

			/*** 새로운 데이터 기록 ***/
			PSN = ((*flashmem)->mapping_table[LBN] * BLOCK_PER_SECTOR) + physical_sector_offset; //실제로 저장된 물리 섹터 번호
			PBN = (*flashmem)->mapping_table[LBN]; //실제로 저장된 물리 블록 번호

			if (Flash_write(flashmem, PSN, data, mapping_method) != 0)
			{
				flash_write_count++; //성공적으로 기록 할 시 카운트 증가
				(*flashmem)->block_free_space[PBN] -= 1;//블록 단위 여유 공간 테이블 감소
			}
		}
		break;

	default:
		return 0;
		break;
	}

	(*flashmem)->save_table(flashmem, mapping_method);
	std::cout << "=====================================================" << std::endl;
	std::cout << "FTL write done" << std::endl;
	std::cout << "총 쓰기 연산 : " << flash_write_count << std::endl;
	std::cout << "총 지우기 연산 : " << flash_erase_count << std::endl;
	if(flash_erase_block_num != -1) std::cout << "삭제된 블록 : " << flash_erase_block_num << "-th block erased" <<std::endl;
	std::cout << "=====================================================" << std::endl;
	return 1;
}