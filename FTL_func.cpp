#include "header.h"

/***
- Print_table, FTL_read, FTL_write ����
- �� ���� ��ȣ �Ǵ� �� ��� ��ȣ�� �������̺�� �����Ͽ� physical_func���� read, write, erase�� �����Ͽ� �۾��� ����
***/

/*************************************************
mapping_method 0 = ���� ���� ���
mapping_method 1 = ��� ���� ���
---

< ���� ���� >
- ��� ���� ũ���� ����� Free Space(���� ����) ���̺� ���� (�ʱⰪ : 32 (��� �� ���Ͱ���), ���� ���� ���� : 0)
- �߰� ������ ���� ��� ������ Free Space ���̺� �� LSN->PSN ��ȯ ���� ���̺� ����
- ������ ��� �� ���� ���Ϳ� 0������ ���������� ���, ���� ���̺� �󿡼� LSN�� �ش� �ϴ� PSN�� ���� �� �ش� ����� Free Space(���� ����) �� ����
- �����͸� ���� �� ���� ��ġ�� ��� �� �ٸ� ���� ���Ϳ� ��� �� ���� ���̺� �󿡼� LSN�� �ش� �ϴ� PSN�� ����
- �ش� ����� ���� á�� ��� ���� ���̺� �󿡼� �������� �ʴ� ������ ������ �������� �ٸ� �� ��Ͽ� ����,�ش� ��� Erase,
�Ű��� ����� Free Space ���̺� �� ����, ���� ���̺� �󿡼� LSN�� �ش� �ϴ� PSN�� ����(Trim)

< ��� ���� >
- �߰� ������ ���� ��� ������ Free Space ���̺�, ��� ���� ���� ���̺�(LBN -> PBN) ����
- ������ ��� ��
1) ����ڰ� �Է��� �� ���� ��ȣ(LSN)�� �������� �� ��� ��ȣ(LBN)�� LSN / ��� �� ���� ����(32)�� ���ϱ�
2) ��� ������ ���� ���̺� ������ ���� ��� ��ȣ(PBN)�� ���(���� ��� 0������ ���������� �Ҵ�)
3) ���� ��� ������ ���� Offset(0~31)�� LSN % ��� �� ���� ����(32)�� �˾Ƴ���
4) �ش� ���� ����(PSN)�� ���

- ���� ��ġ�� ��� �õ� ��
1) �ش� ���� ��ϳ��� ����� �õ��ϴ� ��ġ�� �����͸� ������ ��� �����͸� �� �ٸ� ��Ͽ� ���� �� �ش� ��� Erase
2) ���ο� ������ ���

- ��� �����͵�� ���� �� ��Ͽ� �ٽ� �Է� �õ� �� ���(��, ����ڰ� �Է��� LSN ��ġ�� �̹� �����Ͱ� �ԷµǾ��ְ�, �ش� LSN�� ���� ���� ����� ������ ���� �����Ͱ� �Էµ� ���͵�� ä�������� ���)
�ش� ��Ͽ� ���ؼ��� �� �̻� ��� �Ұ�
************************************************/

void Print_table(FlashMem **flashmem, int mapping_method) //���� ���̺� ���
{
	__int64 table_size = 0; //���� ���̺��� ũ��
	__int64 table_index = 0; //���� ���̺� �ε���

	if (*flashmem == NULL) //�÷��� �޸𸮰� �Ҵ���� �ʾ��� ���
	{
		std::cout << "not initialized" << std::endl;
		return;
	}

	//��� ���� ���� ���� ���� ���̺�, ���� ���̺��� �ε���� �ʾ��� �� ���Ϸκ��� �޸𸮻� �ε�
	if ((*flashmem)->mapping_table == NULL && (*flashmem)->block_free_space == NULL)
		(*flashmem)->load_table(flashmem, mapping_method);

	switch (mapping_method) //���� ��Ŀ� ���� ���� ���̺� ��� ����
	{
	case 0: //���� ���� ���
		table_size=(*flashmem)->calc_sector_size(); //���� ��
		std::cout<<"< Sector Mapping Table (LSN -> PSN) >"<<std::endl;
		break;

	case 1: //��� ���� ���
		table_size=(*flashmem)->calc_block_size(); //��� ��
		std::cout<<"< Block Mapping Table (LBN -> PBN) >"<<std::endl;
		break;

	default:
		return;
	}

	while(table_index < table_size)
	{
		//�Ҵ�� ���鸸 ���
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

int FTL_read(FlashMem **flashmem, __int64 LSN, int mapping_method) //�� ���� �Ǵ� �� ��Ͽ� �ش�Ǵ� �������̺� �� ���� ���� �Ǵ� ���� ����� ��ġ�� �о��
{

	__int64 LBN = 0; //�ش� �� ���Ͱ� ��ġ�ϰ� �ִ� �� ���
	__int64 PBN = 0; //�ش� ���� ���Ͱ� ��ġ�ϰ� �ִ� ���� ���
	__int64 PSN = 0; //������ ����� ���� ���� ��ȣ
	int physical_sector_offset = 0; //��� ���� ���� offset = 0 ~ 31
	char data_output = NULL; //���� ���ͷκ��� �о���� ������

	if (*flashmem == NULL) //�÷��� �޸𸮰� �Ҵ���� �ʾ��� ���
	{
		std::cout << "not initialized" << std::endl;
		return 0;
	}

	if (LSN > (MB_PER_SECTOR * (*flashmem)->get_flashmem_size()) - 1) //���� �ʰ� ����
	{
		std::cout << "out of range error" << std::endl;
		return 0;
	}

	//��� ���� ���� ���� ���� ���̺�, ���� ���̺��� �ε���� �ʾ��� �� ���Ϸκ��� �޸𸮻� �ε�
	if ((*flashmem)->mapping_table == NULL && (*flashmem)->block_free_space == NULL)
		(*flashmem)->load_table(flashmem, mapping_method);

	switch (mapping_method) //���� ��Ŀ� ���� ���� ���Ϳ� ����
	{
	case 0: //���� ���� ��� (LSN -> PSN)
		PSN = (*flashmem)->mapping_table[LSN];
		//LSN�� �����Ǵ� PSN�� ���� ���
		if (PSN == MAPPING_INIT_VALUE)
		{
			std::cout << "no data" << std::endl;
			return 0;
			
		}
		else //�ش�Ǵ� ���� ������ �����͸� �б�
		{
			std::cout << "PSN : " << PSN <<std::endl; //���� ���� ��ȣ ���
			data_output = Flash_read(flashmem, PSN, mapping_method); //������ �б�
			std::cout << data_output << std::endl;
		}
		return 1;

	case 1: //��� ���� ��� (LSN -> LBN -> PBN -> PSN)
		//����ڰ� �Է��� LSN���� LBN�� ���ϰ� �����Ǵ� PBN�� ���� ���� ��ȣ�� ����
		LBN = LSN / BLOCK_PER_SECTOR; //�ش� �� ���Ͱ� ��ġ�ϰ� �ִ� �� ���
		PBN = (*flashmem)->mapping_table[LBN];
		physical_sector_offset = LSN % BLOCK_PER_SECTOR; //��� ���� ���� offset = 0 ~ 31
		PSN = ((*flashmem)->mapping_table[LBN] * BLOCK_PER_SECTOR) + physical_sector_offset; //������ ����� ���� ���� ��ȣ

		//LBN�� �����Ǵ� PBN�� ���� ���
		if (PBN == MAPPING_INIT_VALUE)
		{
			std::cout << "no data" << std::endl;
			return 0;
		}
		else //�ش�Ǵ� ���� ����� ���� ���� ���
		{
			
			data_output = Flash_read(flashmem, PSN, mapping_method); //������ �б�
			if (data_output != NULL)
			{
				std::cout << "PSN : " << PSN << std::endl; //���� ���� ��ȣ ���
				std::cout << data_output << std::endl;
			}
			else
			{
				std::cout << "PSN : " << PSN << std::endl; //���� ���� ��ȣ ���
				std::cout << "no data" << std::endl;
			}
		}
		return 1;

	default:
		return 0;
	}
	return 0;
}

int FTL_write(FlashMem **flashmem, __int64 LSN, char data, int mapping_method) //�� ���� �Ǵ� �� ��Ͽ� �ش�Ǵ� �������̺� �� ���� ���� �Ǵ� ���� ��� ��ġ�� ���
{
	int flash_write_count = 0; //������ ����� Flash_write Ƚ�� ī����
	int flash_erase_count = 0; //������ ����� Flash_erase Ƚ�� ī����
	__int64 flash_erase_block_num = -1; //������ ������ ���� ��� ��ȣ

	char read_buffer[BLOCK_PER_SECTOR] = {NULL}; //�� ��ϳ��� ������ �ӽ� ���� ����
	int read_buffer_index = 0; //�����͸� �о �ӽ������ϱ� ���� read_buffer �ε��� ����
	int physical_sector_offset = 0; //��� ���� ���� offset = 0 ~ 31
	__int64 next_block = 0; //���� ����� ���� �� ��� �����͸� ������ �ٸ� ����� ��ġ
	__int64 PBN = 0; //�ش� ���� ���Ͱ� ��ġ�ϰ� �ִ� ���� ���
	__int64 LBN = 0; //�ش� �� ���Ͱ� ��ġ�ϰ� �ִ� �� ���
	__int64 PSN = 0; //������ ����� ���� ���� ��ȣ

	if (*flashmem == NULL) //�÷��� �޸𸮰� �Ҵ���� �ʾ��� ���
	{
		std::cout << "not initialized" << std::endl;
		return 0;
	}

	if (LSN > (MB_PER_SECTOR * (*flashmem)->get_flashmem_size()) - 1) //���� �ʰ� ����
	{
		std::cout << "out of range error" << std::endl;
		return 0;
	}

	//��� ���� ���� ���� ���� ���̺�, ���� ���̺��� �ε���� �ʾ��� �� ���Ϸκ��� �޸𸮻� �ε�
	if ((*flashmem)->mapping_table == NULL && (*flashmem)->block_free_space == NULL)
		(*flashmem)->load_table(flashmem, mapping_method);

	switch (mapping_method)
	{
	case 0: //���� ���� ���

		/*** ���� ���� ���̺��� �Ҵ� �� ���������� �Ҵ� ***/
		if ((*flashmem)->mapping_table[LSN] == MAPPING_INIT_VALUE)
		{
			for (__int64 i = 0; i < (*flashmem)->calc_block_size(); i++) //�� ���� ���� ����� ã�������� �ݺ�
			{
				if ((*flashmem)->block_free_space[i] > 0)
				{
					PBN = (__int64)i;
					break;
				}
			}
			/*** �ش� ��� ������ �� ���͸� ã�� �� ���� �ݺ� ***/
			PSN = PBN * BLOCK_PER_SECTOR;
			for (int j = 0; j < BLOCK_PER_SECTOR; j++)
			{
				if (Flash_read(flashmem, PSN, mapping_method) == NULL) //�����͸� ��ȯ���� ���� �� �� ����
				{
					(*flashmem)->mapping_table[LSN] = PSN; //�ش� ��ġ�� ����
					break;
				}
				PSN++;
			}
		}
		
		PSN = (*flashmem)->mapping_table[LSN]; //���� ���̺��� �� ���� ��ȣ�� �ش� �Ǵ� ���� ���� ��ȣ
		PBN = PSN / BLOCK_PER_SECTOR; //�ش� ���� ���Ͱ� ��ġ�ϰ� �ִ� ���� ���
		LBN = LSN / BLOCK_PER_SECTOR; //�ش� �� ���Ͱ� ��ġ�ϰ� �ִ� �� ���

		if ((*flashmem)->block_free_space[PBN] == 0) //�ش� ���� ����� ���� á�� ���
		{
			/***
			���� ���̺� �󿡼� �������� �ʴ� ������ ������ �������� �ٸ� �� ��Ͽ� ����
			���� ���̺� �󿡼� LSN�� �ش� �ϴ� PSN�� ����(Trim)
			***/

			__int64 LSN_trim_start = LBN * BLOCK_PER_SECTOR; //�����ϰ��� �ϴ� ���� ���̺� �� �� ������ ���� ��ġ
			__int64 LSN_trim_end = LSN_trim_start + BLOCK_PER_SECTOR; //�����ϰ��� �ϴ� ���� ���̺� �� �� ������ �� ��ġ
			
			read_buffer_index = 0; //�ε��� �ʱ�ȭ
			for (__int64 i = LSN_trim_start; i < LSN_trim_end; i++)
			{
				if ((*flashmem)->mapping_table[i] != MAPPING_INIT_VALUE) //���� ���̺� �󿡼� �����Ǵ� �����͵鸸 �����ϹǷ� read_buffer�� ���������� ����
				{
					(*flashmem)->mapping_table[i] = MAPPING_INIT_VALUE;
					read_buffer[read_buffer_index] = Flash_read(flashmem, (*flashmem)->mapping_table[i], mapping_method);
					read_buffer_index++; //read_buffer�� ���� �ε��� �� ����
				}
			}

			/*** �ش� ����� Erase ***/
			if (Flash_erase(flashmem, PBN, mapping_method) != 0)
			{
				flash_erase_count++; //erase ī��Ʈ ����
				flash_erase_block_num = PBN;
			}

			/*** �� ��� ã�� ***/
			for (__int64 i = 0; i < (*flashmem)->calc_block_size(); i++) //�� ����� ã�������� �ݺ�
			{
				if ((*flashmem)->block_free_space[i] == BLOCK_PER_SECTOR)
				{
					next_block = i;
					break;
				}
			}

			/*** ���� ���̺��� �� ���� ***/
			__int64 PSN_copy_start = next_block * BLOCK_PER_SECTOR; //���� �ű���� �ϴ� ���� ������ ���� ��ġ
			__int64 PSN_copy_end = PSN_copy_start + BLOCK_PER_SECTOR; //���� �ű���� �ϴ� ���� ������ �� ��ġ
			__int64 PSN_copy_start_index = PSN_copy_start; //PSN_copy_start ���� �����ϰ� �ε���

			read_buffer_index = 0; //�ε��� �ʱ�ȭ
			for (__int64 i = LSN_trim_start; i < LSN_trim_end; i++) //���� ���̺� �� �� ���� ��ȣ�� ���� ���� ���� ���� ��ȣ�� �Ҵ�
			{
				if (read_buffer[read_buffer_index] != NULL) //read_buffer�� ��ϵ� ��(�ű���� �ϴ� ����ŭ)��ŭ ���� ���̺� ����
				{
					(*flashmem)->mapping_table[i] = PSN_copy_start_index;
					PSN_copy_start_index++;
					read_buffer_index++;
				}
				else break; //read_buffer�� ó������ NULL�� ���� �� �̻� ����� ���� ����
			}


			/*** �� ������� ������ ���� ***/
			read_buffer_index = 0; //�ε��� �ʱ�ȭ
			for (__int64 i = PSN_copy_start; i < PSN_copy_end; i++)
			{
				if (read_buffer[read_buffer_index] != NULL)
				{
					Flash_write(flashmem, i, read_buffer[read_buffer_index], mapping_method);
					flash_write_count++; //���� ī��Ʈ ����
					(*flashmem)->block_free_space[(*flashmem)->mapping_table[next_block]] -= 1;//��� ���� ���� ���� ���̺� ����
					read_buffer_index++;
				}
				else break; //read_buffer�� ó������ NULL�� ���� �� �̻� ����� ���� ����
			}
		}

		if (Flash_write(flashmem, PSN, data, mapping_method) != 0)
		{
			flash_write_count++; //���������� ��� �� �� ī��Ʈ ����
			(*flashmem)->block_free_space[PBN] -= 1;//��� ���� ���� ���� ���̺� ����
		}
		else //���� ��ġ�� ��� �õ� �� �ٸ� ���Ϳ� ��� �� ���� ���̺� �󿡼� LSN�� �ش� �ϴ� PSN�� ����
		{
			/*** �ش� ��� ������ �� ���͸� ã�� �� ���� �ݺ� ***/
			PSN = PBN * BLOCK_PER_SECTOR;
			for (int i = 0; i < BLOCK_PER_SECTOR; i++)
			{
				if (Flash_read(flashmem, PSN, mapping_method) == NULL) //NULL ��ȯ �� �� ����
				{
					(*flashmem)->mapping_table[LSN] = PSN; //�ش� ��ġ�� ����
					break;
				}
				PSN++;
			}

			if (Flash_write(flashmem, PSN, data, mapping_method) != 0)
			{
				flash_write_count++; //���������� ��� �� �� ī��Ʈ ����
				(*flashmem)->block_free_space[PBN] -= 1;//��� ���� ���� ���� ���̺� ����
			}
		}

		break;

	case 1: //��� ���� ���
		//����ڰ� �Է��� LSN���� LBN�� ���ϰ� �����Ǵ� PBN�� ���� ���� ��ȣ�� ����
		LBN = LSN / BLOCK_PER_SECTOR; //�ش� �� ���Ͱ� ��ġ�ϰ� �ִ� �� ���
		physical_sector_offset = LSN % BLOCK_PER_SECTOR; //��� ���� ���� offset = 0 ~ 31

		/*** ���� ���� ���̺��� �Ҵ� �� ���������� �Ҵ� ***/
		if ((*flashmem)->mapping_table[LBN] == MAPPING_INIT_VALUE)
		{
			for (__int64 i = 0; i < (*flashmem)->calc_block_size(); i++) //�� ����� ã�������� �ݺ�
			{
				if ((*flashmem)->block_free_space[i] == BLOCK_PER_SECTOR)
				{
					(*flashmem)->mapping_table[LBN] = i;
					break;
				}
			}
		}

		PBN = (*flashmem)->mapping_table[LBN]; //������ ����� ���� ��� ��ȣ
		PSN = (PBN * BLOCK_PER_SECTOR) + physical_sector_offset; //������ ����� ���� ���� ��ȣ

		if ((*flashmem)->block_free_space[PBN] == 0) //�ش� ��Ͽ� ���������� ���� ���
		{
			std::cout << "�ش� ���� ��Ͽ� �� �̻� ��� �� �� �����ϴ�." << std::endl;
			return 0;
		}

		if (Flash_write(flashmem, PSN, data, mapping_method) != 0)
		{
			flash_write_count++; //���������� ��� �� �� ī��Ʈ ����
			(*flashmem)->block_free_space[PBN] -= 1;//��� ���� ���� ���� ���̺� ����
		}
		else //�̹� �����Ͱ� ��ϵǾ� �ִ� ���
		{
			//�ش� ���� ��ϳ��� ����� �õ��ϴ� ��ġ�� �����͸� ������ ��� �����͸� �� �ٸ� ��Ͽ� ���� �� �ش� ��� Erase

			for (__int64 i = 0; i < (*flashmem)->calc_block_size(); i++) //�� ����� ã�������� �ݺ�
			{
				if ((*flashmem)->block_free_space[i] == BLOCK_PER_SECTOR)
				{
					next_block = i;
					break;
				}
			}

			/*** �ش� ����� ��� ������ �о� �ӽ����� ***/

			//���� offset�� ��ġ���Ѿ� �ϹǷ� ���Ľ�Ű���ʰ� ������ �о����
			__int64 PSN_trim_pos = (*flashmem)->mapping_table[LBN] * BLOCK_PER_SECTOR; //������ ������ ���� ���� ��ġ
			__int64 PSN_exclude_pos = (*flashmem)->mapping_table[LBN] * BLOCK_PER_SECTOR + physical_sector_offset;//������ ���� (���ο� ���� ����� ��ġ)
			read_buffer_index = 0; //�ε��� �ʱ�ȭ
			for (int i = 0; i < BLOCK_PER_SECTOR; i++)
			{
				if (PSN_trim_pos == PSN_exclude_pos) //���ο� ���� ����� ���� ����
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

			/*** �ش� ����� Erase ***/
			if (Flash_erase(flashmem, (*flashmem)->mapping_table[LBN], mapping_method) != 0)
			{
				flash_erase_count++; //erase ī��Ʈ ����
				flash_erase_block_num = (*flashmem)->mapping_table[LBN];
			}

			/*** ���� ���̺��� �� ���� ***/
			(*flashmem)->mapping_table[LBN] = next_block;
			PBN = (*flashmem)->mapping_table[LBN]; //������ ����� ���� ��� ��ȣ

			/*** �� ��Ͽ� ���� ***/
			__int64 copy_start_pos = next_block * BLOCK_PER_SECTOR;

			for (int i = 0; i < BLOCK_PER_SECTOR; i++)
			{
				if (read_buffer[i] != NULL) //���� ���ͷκ��� �о���θ�ŭ ���
				{
					if (Flash_write(flashmem, copy_start_pos, read_buffer[i], mapping_method) != 0)
					{
						flash_write_count++; //���� ī��Ʈ ����
						copy_start_pos++;
						(*flashmem)->block_free_space[PBN] -= 1;//��� ���� ���� ���� ���̺� ����
					}
				}
				else copy_start_pos++;
			}

			/*** ���ο� ������ ��� ***/
			PSN = ((*flashmem)->mapping_table[LBN] * BLOCK_PER_SECTOR) + physical_sector_offset; //������ ����� ���� ���� ��ȣ
			PBN = (*flashmem)->mapping_table[LBN]; //������ ����� ���� ��� ��ȣ

			if (Flash_write(flashmem, PSN, data, mapping_method) != 0)
			{
				flash_write_count++; //���������� ��� �� �� ī��Ʈ ����
				(*flashmem)->block_free_space[PBN] -= 1;//��� ���� ���� ���� ���̺� ����
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
	std::cout << "�� ���� ���� : " << flash_write_count << std::endl;
	std::cout << "�� ����� ���� : " << flash_erase_count << std::endl;
	if(flash_erase_block_num != -1) std::cout << "������ ��� : " << flash_erase_block_num << "-th block erased" <<std::endl;
	std::cout << "=====================================================" << std::endl;
	return 1;
}