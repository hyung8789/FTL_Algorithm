#include "header.h"

//��ɾ� ��� ���, �÷��� �޸��� ����, ���� ���̺� �ҷ����� �� ���丮�� �뷮�� ����ϴ� ���� �Լ� ����

FlashMem::FlashMem()
{
	flashmem_size = 0;
	mapping_table = NULL;
	block_free_space = NULL;
}

FlashMem::FlashMem(__int64 megabytes) //megabytes ũ���� �÷��� �޸� ����
{
	flashmem_size = megabytes;
	mapping_table = NULL;
	block_free_space = NULL;
}

void FlashMem::chk_initialized(FlashMem **flash, int &mapping_method) //�÷��� �޸𸮰� �̹� �����Ǿ� �ִ��� Ȯ��
{
	if ((*flash) == NULL)
	{
		FILE *volume = NULL;

		if ((volume = fopen("volume.txt", "rt")) == NULL) //�б� + �ؽ�Ʈ���� ���
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
					std::cin.clear(); //������Ʈ�� �ʱ�ȭ
					std::cin.ignore(INT_MAX, '\n'); //�Է¹��ۺ���
					break;
				}
				else if (input < 0 || input > 1)
				{
					std::cin.clear(); //������Ʈ�� �ʱ�ȭ
					std::cin.ignore(INT_MAX, '\n'); //�Է¹��ۺ���
				}
				else //������ ����� �÷��� �޸��� �뷮 �ҷ��ͼ� ���Ҵ�
				{
					__int64 megabytes;
					fscanf(volume,"%I64d\n",&megabytes);
					(*flash) = new FlashMem(megabytes); //�뷮 ����
					fscanf(volume, "%d", &mapping_method); //���� ��� ����
					std::cin.clear(); //������Ʈ�� �ʱ�ȭ
					std::cin.ignore(INT_MAX, '\n'); //�Է¹��ۺ���
				 	break;
				}
			}
			if (volume != NULL)	fclose(volume);
			return;
		}
	}
	else return;
}

void FlashMem::disp_command(int mapping_method) //Ŀ�ǵ� ��ɾ� ���
{
	switch (mapping_method)
	{
	case 0:
		system("cls");
		std::cout << "=====================================================" << std::endl;
		std::cout << "	�÷��� �޸� �ùķ����� - Sector Mapping Method" << std::endl;
		std::cout << "=====================================================" << std::endl;
		std::cout << " init x �Ǵ� i x - x MB ��ŭ �÷��� �޸� ���� " << std::endl;
		std::cout << " read LSN �Ǵ� r LSN - �� ������ ������ �б�" << std::endl;
		std::cout << " write LSN data �Ǵ� w LSN data - �� ������ data ���" << std::endl;
		std::cout << " change - ���� ��� ����" << std::endl;
		std::cout << " print - ���� ���̺� ���" << std::endl;
		std::cout << "=====================================================" << std::endl;
		break;

	case 1:
		system("cls");
		std::cout << "=====================================================" << std::endl;
		std::cout << "	�÷��� �޸� �ùķ����� - Block Mapping Method" << std::endl;
		std::cout << "=====================================================" << std::endl;
		std::cout << " init x �Ǵ� i x - x MB ��ŭ �÷��� �޸� ���� " << std::endl;
		std::cout << " read LSN �Ǵ� r LSN - �� ������ ������ �б�" << std::endl;
		std::cout << " write LSN data �Ǵ� w LSN data - �� ������ data ���" << std::endl;
		std::cout << " change - ���� ��� ����" << std::endl;
		std::cout << " print - ���� ���̺� ���" << std::endl;
		std::cout << "=====================================================" << std::endl;
		break;

	case 2:
		system("cls");
		std::cout << "=====================================================" << std::endl;
		std::cout << "	�÷��� �޸� �ùķ�����" << std::endl;
		std::cout << "=====================================================" << std::endl;
		std::cout << " init x �Ǵ� i x - x MB ��ŭ �÷��� �޸� ���� " << std::endl;
		std::cout << " read PSN �Ǵ� r PSN - ���� ������ ������ �б�" << std::endl;
		std::cout << " write PSN data �Ǵ� w PSN data - ���� ������ data ���" << std::endl;
		std::cout << " erase PBN �Ǵ� e PBN - ���� ����� ������ ����" << std::endl;
		std::cout << " change - ���� ��� ����" << std::endl;
		std::cout << "=====================================================" << std::endl;
		break;
	}
}

void FlashMem::input_command(FlashMem **flash, int &mapping_method) //Ŀ�ǵ� ��ɾ� �Է�
{
	std::string user_input; //����ڷκ��� ��ɾ �� �ٷ� �Է¹޴� ����
	///����ڷκ��� �Է¹޴� ����
	std::string command; //��ɾ�
	char data=NULL; //����ϰ��� �ϴ� ������
	char data_output = NULL; //���� ���ͷκ��� �о���� ������
	__int64 megabytes = -1; //�����ϰ��� �ϴ� �÷��� �޸� �뷮
	__int64 LSN = -1; //�� ���� ��ȣ
	__int64 LBN = -1; //�� ��� ��ȣ
	__int64 PBN = -1; //���� ��� ��ȣ
	__int64 PSN = -1; //���� ���� ��ȣ

		
	std::cout << "��ɾ� >> ";
	std::getline(std::cin, user_input); //�� �ٷ� �Է¹ޱ�
	std::stringstream ss(user_input); //�и�
	ss >> command;

	switch(mapping_method)
	{
	case 2: //���� ������� ����
		if (command.compare("init") == 0 || command.compare("i") == 0) //megabytes ��ŭ �÷��� �޸� ����
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
		else if (command.compare("read") == 0 || command.compare("r") == 0) //���� ������ ������ �б�
		{
			ss >> PSN;
			if (PSN < 0)
			{
				std::cout << "�߸��� ��ɾ� �Է�" << std::endl;
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
		else if (command.compare("write") == 0 || command.compare("w") == 0) //���� ���Ϳ� data ���
		{
			ss >> PSN;
			ss >> data;
			if (PSN < 0 || data == NULL)
			{
				std::cout << "�߸��� ��ɾ� �Է�" << std::endl;
				system("pause");
				break;
			}
			Flash_write(flash, PSN, data, mapping_method);
			system("pause");
		}
		else if (command.compare("erase") == 0 || command.compare("e") == 0) //���� ��� ��ȣ�� �ش�Ǵ� ����� ������ ����
		{
			ss >> PBN;
			if (PBN < 0)
			{
				std::cout << "�߸��� ��ɾ� �Է�" << std::endl;
				system("pause");
				break;
			}
			Flash_erase(flash, PBN, mapping_method);
			system("pause");
		}
		else if (command.compare("change") == 0) //���� ��� ����
		{
			mapping_method = (*flash)->switch_mapping_method();
			if((*flash) != NULL)
			{
				//���̺� ���� ����
				if ((*flash)->mapping_table != NULL)
					(*flash)->mapping_table = NULL;
				if ((*flash)->block_free_space != NULL)
					(*flash)->block_free_space = NULL;
			}
		}
		else
		{
			std::cout << "�߸��� ��ɾ� �Է�" << std::endl;
			system("pause");
			break;
		}
		break;

	default: //���� ���� �Ǵ� ��� ���� ���
		if (command.compare("init") == 0 || command.compare("i") == 0) //megabytes ��ŭ �÷��� �޸� ����
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
		else if (command.compare("read") == 0 || command.compare("r") == 0) //�� ������ ������ �б�
		{
			ss >> LSN;
			if (LSN < 0)
			{
				std::cout << "�߸��� ��ɾ� �Է�" << std::endl;
				system("pause");
				break;
			}
			FTL_read(flash, LSN, mapping_method);
			system("pause");
			break;
			
		}
		else if (command.compare("write") == 0 || command.compare("w") == 0) //�� ���Ϳ� data ���
		{
			ss >> LSN;
			ss >> data;
			if (LSN < 0 || data == NULL)
			{
				std::cout << "�߸��� ��ɾ� �Է�" << std::endl;
				system("pause");
				break;
			}
			FTL_write(flash, LSN, data, mapping_method);
			system("pause");
		}
		else if (command.compare("change") == 0) //���� ��� ����
		{
			mapping_method = (*flash)->switch_mapping_method();
			if ((*flash) != NULL)
			{
				//���̺� ���� ����
				if ((*flash)->mapping_table != NULL)
					(*flash)->mapping_table = NULL;
				if ((*flash)->block_free_space != NULL)
					(*flash)->block_free_space = NULL;
			}
		}
		else if (command.compare("print") == 0) //���� ���̺� ���
		{
			Print_table(flash, mapping_method);
			system("pause");
		}
		else
		{
			std::cout << "�߸��� ��ɾ� �Է�" << std::endl;
			system("pause");
			break;
		}
		break;
	}
}
void FlashMem::disp_flash_info(FlashMem **flash,int mapping_method) //���� ������ �÷��� �޸��� ���� ���
{
	if (*flash != NULL) //���� ������ �÷��� �޸��� ���� �����ֱ�
	{
		__int64 block_size = (*flash)->calc_block_size(); //��� ����
		__int64 sector_size = (*flash)->calc_sector_size(); //���� ����

		std::cout << "���� ������ �÷��� �޸��� �뷮 : " << (*flash)->get_flashmem_size() << "MB(" << (*flash)->calc_storage_byte() << "bytes)" << std::endl;
		std::cout << "��� �� : " << block_size << " [���� : 0~" << block_size - 1 << "]" << std::endl;
		std::cout << "���� �� : " << sector_size << " [���� : 0~" << sector_size - 1 << "]" << std::endl;
		std::cout << "=====================================================" << std::endl;
	}
	return;
}

int FlashMem::switch_mapping_method() //���� �÷��� �޸��� ���� ��� ����
{
	while (1)
	{
		int input = 0;

		system("cls");
		std::cout << "==============================================================" << std::endl;
		std::cout << "!! ���� ��� ���� �� ���� �÷��� �޸𸮸� ����(init)�Ͽ��� ��" << std::endl;
		std::cout << "==============================================================" << std::endl;
		std::cout << "0 : Sector Mapping Method" << std::endl;
		std::cout << "1 : Block Mapping Method" << std::endl;
		std::cout << "2 : Do not use any Mapping Method" << std::endl;
		std::cout << "==============================================================" << std::endl;
		std::cout << ">>";
		std::cin >> input;

		if(input < 0 || input > 2)
		{
			std::cin.clear(); //������Ʈ�� �ʱ�ȭ
			std::cin.ignore(INT_MAX, '\n'); //�Է¹��ۺ���
			continue;
		}
		std::cin.clear(); //������Ʈ�� �ʱ�ȭ
		std::cin.ignore(INT_MAX, '\n'); //�Է¹��ۺ���
		
		return input;
	}
}

void FlashMem::load_table(FlashMem **flashmem, int mapping_method) //���� ��Ŀ� ���� ���� ���̺� �Ǵ� ��� ���� �������� ���̺� �ε�
{
	FILE *table = NULL;
	__int64 *mapping_table = NULL; //���� ���� �Ǵ� ��� ���� ���̺�
	__int64 *block_free_space = NULL; //��� �� ���� ���������� �����ϴ� ���̺�

	__int64 block_size = (*flashmem)->calc_block_size(); //��ü ��� ����
	__int64 sector_size = (*flashmem)->calc_sector_size(); //��ü ���� ����

	if (*flashmem == NULL) //�÷��� �޸𸮰� �Ҵ���� �ʾ��� ���
	{
		std::cout << "not initialized" << std::endl;
		return;
	}

	if ((table = fopen("table.bin", "rb")) == NULL) //�б� + �������� ���
	{
		fprintf(stderr, "table.bin ������ �б���� �� �� �����ϴ�.");
		return;
	}
	
	if ((*flashmem)->block_free_space != NULL && (*flashmem)->mapping_table != NULL) //����Ǿ� ���� ��� �޸� ���� �� ��ũ �ʱ�ȭ
	{
		//�޸� ����
		delete[](*flashmem)->block_free_space;
		delete[](*flashmem)->mapping_table;

		//��ũ �ʱ�ȭ
		(*flashmem)->block_free_space = NULL;
		(*flashmem)->mapping_table = NULL;
	}

	switch(mapping_method)
	{
	case 0: //���� ���� ���
		//��ϴ����� �ش� ����� ���������� ���
		block_free_space = new __int64[block_size];
		for (__int64 i = 0; i < block_size; i++) //0~31�� ���� ����, BLOCK_PER_SECTOR�� ��ü �ʱ�ȭ
		{
			block_free_space[i] = BLOCK_PER_SECTOR;
		}

		//���� ���� ���� ���̺�(LSN -> PSN) ����
		mapping_table = new __int64[sector_size]; //���� ���� ũ��� ����
		for (__int64 i = 0; i < sector_size; i++)
		{
			mapping_table[i] = MAPPING_INIT_VALUE;
		}

		//��� ������ �ش� ����� Free Space(���� ����) ���̺�,���� ���� LSN->PSN ��ȯ ���� ���̺� �ҷ�����
		fread(block_free_space, sizeof(__int64), block_size, table); //��� ������ �������� ���� ���̺� �ҷ�����
		fread(mapping_table, sizeof(__int64), sector_size, table); //���� ���̺� �ҷ�����
		break;
	case 1: //��� ���� ���
			//��ϴ����� �ش� ����� ���������� ���
		block_free_space = new __int64[block_size];
		for (__int64 i = 0; i < block_size; i++)
		{
			block_free_space[i] = BLOCK_PER_SECTOR;
		}

		//��� ���� ���� ���̺�(LBN -> PBN) ����
		mapping_table = new __int64[block_size]; //��� ���� ũ��� ����
		for (__int64 i = 0; i < block_size; i++)
		{
			mapping_table[i] = MAPPING_INIT_VALUE;
		}

		fread(block_free_space, sizeof(__int64), block_size, table); //��� ������ �������� ���� ���̺� �ҷ�����
		fread(mapping_table, sizeof(__int64), block_size, table); //���� ���̺� �ҷ�����
		break;
	default:
		break;
	}
	(*flashmem)->block_free_space = block_free_space; //��� ������ �������� ���� ���̺� ����
	(*flashmem)->mapping_table = mapping_table; //���� ���̺� ����

	fclose(table);
	return;
}

void FlashMem::save_table(FlashMem **flashmem, int mapping_method) //���� ��Ŀ� ���� ���� ���̺� �Ǵ� ��� ���� �������� ���̺� ����
{
	FILE *table = NULL;

	if (*flashmem == NULL) //�÷��� �޸𸮰� �Ҵ���� �ʾ��� ���
	{
		std::cout << "not initialized" << std::endl;
		return;
	}

	if ((table = fopen("table.bin", "wb")) == NULL) //���� + �������� ���
	{
		fprintf(stderr, "table.bin ������ �б���� �� �� �����ϴ�.");
		return;
	}

	switch (mapping_method)
	{
	case 0: //���� ���� ���
		//��� ������ �ش� ����� Free Space(���� ����) ���̺�, ���� ���� LSN->PSN ��ȯ ���� ���̺� ����
		fwrite((*flashmem)->block_free_space, sizeof(__int64), (*flashmem)->calc_block_size(), table); //��� ������ �������� ���� ���̺� ���
		fwrite((*flashmem)->mapping_table, sizeof(__int64), (*flashmem)->calc_sector_size(), table); //���� ���̺� ���
		break;
	case 1: //��� ���� ���
		//��� ������ �ش� ����� Free Space(���� ����) ���̺�, ��� ���� ���� ���̺�(LBN->PBN) ����
		fwrite((*flashmem)->block_free_space, sizeof(__int64), (*flashmem)->calc_block_size(), table); //��� ������ �������� ���� ���̺� ���
		fwrite((*flashmem)->mapping_table, sizeof(__int64), (*flashmem)->calc_block_size(), table); //���� ���̺� ���
		break;
	default:
		break;
	}

	fclose(table);
	return;
}

__int64 FlashMem::get_flashmem_size() //�÷��� �޸� ũ�� ��ȯ
{
	return flashmem_size;
}

__int64 FlashMem::calc_block_size() //�Ҵ�� �޸� ũ�⿡ �ش��ϴ� ����� ���� ��ȯ
{
	return flashmem_size*MB_PER_BLOCK;
}

__int64 FlashMem::calc_sector_size() //�Ҵ�� �޸� ũ�⿡ �ش��ϴ� ������ ���� ��ȯ
{
	return calc_block_size()*BLOCK_PER_SECTOR;
}

__int64 FlashMem::calc_storage_byte() //�Ҵ�� �޸� ũ�⿡ �ش��ϴ� �� byte ũ�⸦ ��ȯ
{
	return calc_sector_size()*BYTE_PER_SECTOR;
}