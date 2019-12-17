#include "header.h"

//명령어 목록 출력, 플래시 메모리의 생성, 매핑 테이블 불러오기 및 스토리지 용량을 계산하는 내부 함수 정의

FlashMem::FlashMem()
{
	flashmem_size = 0;
	mapping_table = NULL;
	block_free_space = NULL;
}

FlashMem::FlashMem(__int64 megabytes) //megabytes 크기의 플래시 메모리 생성
{
	flashmem_size = megabytes;
	mapping_table = NULL;
	block_free_space = NULL;
}

void FlashMem::chk_initialized(FlashMem **flash, int &mapping_method) //플래시 메모리가 이미 생성되어 있는지 확인
{
	if ((*flash) == NULL)
	{
		FILE *volume = NULL;

		if ((volume = fopen("volume.txt", "rt")) == NULL) //읽기 + 텍스트파일 모드
		{
			return;
		}
		else
		{
			while (1)
			{
				system("cls");
				int input = 0;
				std::cout << "already initialized continue?" << std::endl;
				std::cout << "0: ignore, 1: continue" << std::endl;
				std::cin >> input;
				if (input == 0)
				{
					std::cin.clear(); //오류스트림 초기화
					std::cin.ignore(INT_MAX, '\n'); //입력버퍼비우기
					break;
				}
				else if (input < 0 || input > 1)
				{
					std::cin.clear(); //오류스트림 초기화
					std::cin.ignore(INT_MAX, '\n'); //입력버퍼비우기
				}
				else //기존에 저장된 플래시 메모리의 용량 불러와서 재할당
				{
					__int64 megabytes;
					fscanf(volume,"%I64d\n",&megabytes);
					(*flash) = new FlashMem(megabytes); //용량 설정
					fscanf(volume, "%d", &mapping_method); //매핑 방식 설정
					std::cin.clear(); //오류스트림 초기화
					std::cin.ignore(INT_MAX, '\n'); //입력버퍼비우기
				 	break;
				}
			}
			if (volume != NULL)	fclose(volume);
			return;
		}
	}
	else return;
}

void FlashMem::disp_command(int mapping_method) //커맨드 명령어 출력
{
	switch (mapping_method)
	{
	case 0:
		system("cls");
		std::cout << "=====================================================" << std::endl;
		std::cout << "	플래시 메모리 시뮬레이터 - Sector Mapping Method" << std::endl;
		std::cout << "=====================================================" << std::endl;
		std::cout << " init x 또는 i x - x MB 만큼 플래시 메모리 생성 " << std::endl;
		std::cout << " read LSN 또는 r LSN - 논리 섹터의 데이터 읽기" << std::endl;
		std::cout << " write LSN data 또는 w LSN data - 논리 섹터의 data 기록" << std::endl;
		std::cout << " change - 매핑 방식 변경" << std::endl;
		std::cout << " print - 매핑 테이블 출력" << std::endl;
		std::cout << "=====================================================" << std::endl;
		break;

	case 1:
		system("cls");
		std::cout << "=====================================================" << std::endl;
		std::cout << "	플래시 메모리 시뮬레이터 - Block Mapping Method" << std::endl;
		std::cout << "=====================================================" << std::endl;
		std::cout << " init x 또는 i x - x MB 만큼 플래시 메모리 생성 " << std::endl;
		std::cout << " read LSN 또는 r LSN - 논리 섹터의 데이터 읽기" << std::endl;
		std::cout << " write LSN data 또는 w LSN data - 논리 섹터의 data 기록" << std::endl;
		std::cout << " change - 매핑 방식 변경" << std::endl;
		std::cout << " print - 매핑 테이블 출력" << std::endl;
		std::cout << "=====================================================" << std::endl;
		break;

	case 2:
		system("cls");
		std::cout << "=====================================================" << std::endl;
		std::cout << "	플래시 메모리 시뮬레이터" << std::endl;
		std::cout << "=====================================================" << std::endl;
		std::cout << " init x 또는 i x - x MB 만큼 플래시 메모리 생성 " << std::endl;
		std::cout << " read PSN 또는 r PSN - 물리 섹터의 데이터 읽기" << std::endl;
		std::cout << " write PSN data 또는 w PSN data - 물리 섹터의 data 기록" << std::endl;
		std::cout << " erase PBN 또는 e PBN - 물리 블록의 데이터 삭제" << std::endl;
		std::cout << " change - 매핑 방식 변경" << std::endl;
		std::cout << "=====================================================" << std::endl;
		break;
	}
}

void FlashMem::input_command(FlashMem **flash, int &mapping_method) //커맨드 명령어 입력
{
	std::string user_input; //사용자로부터 명령어를 한 줄로 입력받는 변수
	///사용자로부터 입력받는 값들
	std::string command; //명령어
	char data=NULL; //기록하고자 하는 데이터
	char data_output = NULL; //물리 섹터로부터 읽어들인 데이터
	__int64 megabytes = -1; //생성하고자 하는 플래시 메모리 용량
	__int64 LSN = -1; //논리 섹터 번호
	__int64 LBN = -1; //논리 블록 번호
	__int64 PBN = -1; //물리 블록 번호
	__int64 PSN = -1; //물리 섹터 번호

		
	std::cout << "명령어 >> ";
	std::getline(std::cin, user_input); //한 줄로 입력받기
	std::stringstream ss(user_input); //분리
	ss >> command;

	switch(mapping_method)
	{
	case 2: //매핑 사용하지 않음
		if (command.compare("init") == 0 || command.compare("i") == 0) //megabytes 만큼 플래시 메모리 생성
		{
			ss >> megabytes;
			if (megabytes <= 0)
			{
				std::cout << "must over 0 megabytes" << std::endl;
				system("pause");
				break;
			}
			init(flash, megabytes, mapping_method);
			system("pause");
		}
		else if (command.compare("read") == 0 || command.compare("r") == 0) //물리 섹터의 데이터 읽기
		{
			ss >> PSN;
			if (PSN < 0)
			{
				std::cout << "잘못된 명령어 입력" << std::endl;
				system("pause");
				break;
			}
			data_output = Flash_read(flash, PSN, mapping_method);
			if (data_output != NULL)
			{
				std::cout << data_output << std::endl;
			}
			else
			{
				std::cout << "no data" << std::endl;
			}
			system("pause");
			break;
			
		}
		else if (command.compare("write") == 0 || command.compare("w") == 0) //물리 섹터에 data 기록
		{
			ss >> PSN;
			ss >> data;
			if (PSN < 0 || data == NULL)
			{
				std::cout << "잘못된 명령어 입력" << std::endl;
				system("pause");
				break;
			}
			Flash_write(flash, PSN, data, mapping_method);
			system("pause");
		}
		else if (command.compare("erase") == 0 || command.compare("e") == 0) //물리 블록 번호에 해당되는 블록의 데이터 삭제
		{
			ss >> PBN;
			if (PBN < 0)
			{
				std::cout << "잘못된 명령어 입력" << std::endl;
				system("pause");
				break;
			}
			Flash_erase(flash, PBN, mapping_method);
			system("pause");
		}
		else if (command.compare("change") == 0) //매핑 방식 변경
		{
			mapping_method = (*flash)->switch_mapping_method();
			if((*flash) != NULL)
			{
				//테이블 연결 해제
				if ((*flash)->mapping_table != NULL)
					(*flash)->mapping_table = NULL;
				if ((*flash)->block_free_space != NULL)
					(*flash)->block_free_space = NULL;
			}
		}
		else
		{
			std::cout << "잘못된 명령어 입력" << std::endl;
			system("pause");
			break;
		}
		break;

	default: //섹터 매핑 또는 블록 매핑 사용
		if (command.compare("init") == 0 || command.compare("i") == 0) //megabytes 만큼 플래시 메모리 생성
		{
			ss >> megabytes;
			if (megabytes <= 0)
			{
				std::cout << "must over 0 megabytes" << std::endl;
				system("pause");
				break;
			}
			init(flash, megabytes, mapping_method);
			system("pause");
		}
		else if (command.compare("read") == 0 || command.compare("r") == 0) //논리 섹터의 데이터 읽기
		{
			ss >> LSN;
			if (LSN < 0)
			{
				std::cout << "잘못된 명령어 입력" << std::endl;
				system("pause");
				break;
			}
			FTL_read(flash, LSN, mapping_method);
			system("pause");
			break;
			
		}
		else if (command.compare("write") == 0 || command.compare("w") == 0) //논리 섹터에 data 기록
		{
			ss >> LSN;
			ss >> data;
			if (LSN < 0 || data == NULL)
			{
				std::cout << "잘못된 명령어 입력" << std::endl;
				system("pause");
				break;
			}
			FTL_write(flash, LSN, data, mapping_method);
			system("pause");
		}
		else if (command.compare("change") == 0) //매핑 방식 변경
		{
			mapping_method = (*flash)->switch_mapping_method();
			if ((*flash) != NULL)
			{
				//테이블 연결 해제
				if ((*flash)->mapping_table != NULL)
					(*flash)->mapping_table = NULL;
				if ((*flash)->block_free_space != NULL)
					(*flash)->block_free_space = NULL;
			}
		}
		else if (command.compare("print") == 0) //매핑 테이블 출력
		{
			Print_table(flash, mapping_method);
			system("pause");
		}
		else
		{
			std::cout << "잘못된 명령어 입력" << std::endl;
			system("pause");
			break;
		}
		break;
	}
}
void FlashMem::disp_flash_info(FlashMem **flash,int mapping_method) //현재 생성된 플래시 메모리의 정보 출력
{
	if (*flash != NULL) //현재 생성된 플래시 메모리의 정보 보여주기
	{
		__int64 block_size = (*flash)->calc_block_size(); //블록 개수
		__int64 sector_size = (*flash)->calc_sector_size(); //섹터 개수

		std::cout << "현재 생성된 플래시 메모리의 용량 : " << (*flash)->get_flashmem_size() << "MB(" << (*flash)->calc_storage_byte() << "bytes)" << std::endl;
		std::cout << "블록 수 : " << block_size << " [범위 : 0~" << block_size - 1 << "]" << std::endl;
		std::cout << "섹터 수 : " << sector_size << " [범위 : 0~" << sector_size - 1 << "]" << std::endl;
		std::cout << "=====================================================" << std::endl;
	}
	return;
}

int FlashMem::switch_mapping_method() //현재 플래시 메모리의 매핑 방식 변경
{
	while (1)
	{
		int input = 0;

		system("cls");
		std::cout << "==============================================================" << std::endl;
		std::cout << "!! 매핑 방식 변경 시 새로 플래시 메모리를 생성(init)하여야 함" << std::endl;
		std::cout << "==============================================================" << std::endl;
		std::cout << "0 : Sector Mapping Method" << std::endl;
		std::cout << "1 : Block Mapping Method" << std::endl;
		std::cout << "2 : Do not use any Mapping Method" << std::endl;
		std::cout << "==============================================================" << std::endl;
		std::cout << ">>";
		std::cin >> input;

		if(input < 0 || input > 2)
		{
			std::cin.clear(); //오류스트림 초기화
			std::cin.ignore(INT_MAX, '\n'); //입력버퍼비우기
			continue;
		}
		std::cin.clear(); //오류스트림 초기화
		std::cin.ignore(INT_MAX, '\n'); //입력버퍼비우기
		
		return input;
	}
}

void FlashMem::load_table(FlashMem **flashmem, int mapping_method) //매핑 방식에 따른 매핑 테이블 또는 블록 단위 여유공간 테이블 로드
{
	FILE *table = NULL;
	__int64 *mapping_table = NULL; //섹터 매핑 또는 블록 매핑 테이블
	__int64 *block_free_space = NULL; //블록 당 섹터 여유공간을 저장하는 테이블

	__int64 block_size = (*flashmem)->calc_block_size(); //전체 블록 개수
	__int64 sector_size = (*flashmem)->calc_sector_size(); //전체 섹터 개수

	if (*flashmem == NULL) //플래시 메모리가 할당되지 않았을 경우
	{
		std::cout << "not initialized" << std::endl;
		return;
	}

	if ((table = fopen("table.bin", "rb")) == NULL) //읽기 + 이진파일 모드
	{
		fprintf(stderr, "table.bin 파일을 읽기모드로 열 수 없습니다.");
		return;
	}
	
	if ((*flashmem)->block_free_space != NULL && (*flashmem)->mapping_table != NULL) //연결되어 있을 경우 메모리 해제 및 링크 초기화
	{
		//메모리 해제
		delete[](*flashmem)->block_free_space;
		delete[](*flashmem)->mapping_table;

		//링크 초기화
		(*flashmem)->block_free_space = NULL;
		(*flashmem)->mapping_table = NULL;
	}

	switch(mapping_method)
	{
	case 0: //섹터 매핑 방식
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

		//블록 단위로 해당 블록의 Free Space(여유 공간) 테이블,섹터 단위 LSN->PSN 변환 매핑 테이블 불러오기
		fread(block_free_space, sizeof(__int64), block_size, table); //블록 단위의 여유공간 관리 테이블 불러오기
		fread(mapping_table, sizeof(__int64), sector_size, table); //매핑 테이블 불러오기
		break;
	case 1: //블록 매핑 방식
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

		fread(block_free_space, sizeof(__int64), block_size, table); //블록 단위의 여유공간 관리 테이블 불러오기
		fread(mapping_table, sizeof(__int64), block_size, table); //매핑 테이블 불러오기
		break;
	default:
		break;
	}
	(*flashmem)->block_free_space = block_free_space; //블록 단위의 여유공간 관리 테이블 연결
	(*flashmem)->mapping_table = mapping_table; //매핑 테이블 연결

	fclose(table);
	return;
}

void FlashMem::save_table(FlashMem **flashmem, int mapping_method) //매핑 방식에 따른 매핑 테이블 또는 블록 단위 여유공간 테이블 저장
{
	FILE *table = NULL;

	if (*flashmem == NULL) //플래시 메모리가 할당되지 않았을 경우
	{
		std::cout << "not initialized" << std::endl;
		return;
	}

	if ((table = fopen("table.bin", "wb")) == NULL) //쓰기 + 이진파일 모드
	{
		fprintf(stderr, "table.bin 파일을 읽기모드로 열 수 없습니다.");
		return;
	}

	switch (mapping_method)
	{
	case 0: //섹터 매핑 방식
		//블록 단위로 해당 블록의 Free Space(여유 공간) 테이블, 섹터 단위 LSN->PSN 변환 매핑 테이블 저장
		fwrite((*flashmem)->block_free_space, sizeof(__int64), (*flashmem)->calc_block_size(), table); //블록 단위의 여유공간 관리 테이블 기록
		fwrite((*flashmem)->mapping_table, sizeof(__int64), (*flashmem)->calc_sector_size(), table); //매핑 테이블 기록
		break;
	case 1: //블록 매핑 방식
		//블록 단위로 해당 블록의 Free Space(여유 공간) 테이블, 블록 단위 매핑 테이블(LBN->PBN) 저장
		fwrite((*flashmem)->block_free_space, sizeof(__int64), (*flashmem)->calc_block_size(), table); //블록 단위의 여유공간 관리 테이블 기록
		fwrite((*flashmem)->mapping_table, sizeof(__int64), (*flashmem)->calc_block_size(), table); //매핑 테이블 기록
		break;
	default:
		break;
	}

	fclose(table);
	return;
}

__int64 FlashMem::get_flashmem_size() //플래시 메모리 크기 반환
{
	return flashmem_size;
}

__int64 FlashMem::calc_block_size() //할당된 메모리 크기에 해당하는 블록의 개수 반환
{
	return flashmem_size*MB_PER_BLOCK;
}

__int64 FlashMem::calc_sector_size() //할당된 메모리 크기에 해당하는 섹터의 개수 반환
{
	return calc_block_size()*BLOCK_PER_SECTOR;
}

__int64 FlashMem::calc_storage_byte() //할당된 메모리 크기에 해당하는 총 byte 크기를 반환
{
	return calc_sector_size()*BYTE_PER_SECTOR;
}