#include "header.h"

/***
- init, read, write, erase �Լ� ����
- �÷��� �޸𸮿� ���� ���������� �����Ͽ� �۾�
***/

int init(FlashMem **flashmem, __int64 megabytes,int mapping_method) //megabytes ũ���� �÷��� �޸𸮸� �Ҵ�
{
	FILE *fp = NULL;
	FILE *volume = NULL;
	FILE *table = NULL;

	char *data_array = NULL; //���Ͽ� ����ϱ� ���� ����,��� �Ǵ� MB������ �迭
	__int64 *mapping_table = NULL; //���� ���� �Ǵ� ��� ���� ���̺�
	__int64 *block_free_space = NULL; //��� �� ���� ���������� �����ϴ� ���̺�
	
	__int64 initialized_space_size = 0; //���� ����� ���� data_array�� ������
	__int64 storage_byte = 0; //���� ����� ���� ������ ��������� �� byte ũ��
	__int64 block_size = 0; //�Ҵ�� �÷��ø޸��� ��ü ��� ����
	__int64 sector_size = 0; //�Ҵ�� �÷��ø޸��� ��ü ���� ����
	__int64 init_pos_next = 0; //���� �������� ���� ��ġ

	//�̹� �÷��� �޸𸮸� �����Ͽ��� ��� �ٽ� �̴ϼȶ���¡�� �����
	*flashmem = new FlashMem(megabytes);

	storage_byte = (*flashmem)->calc_storage_byte(); //������ ���� ������ �� byte ũ��
	block_size = (*flashmem)->calc_block_size(); //��ü ��� ����
	sector_size = (*flashmem)->calc_sector_size(); //��ü ���� ����

	data_array = new char[MB_PER_BYTE]; //1MB ũ���� ������� ����
	memset(data_array, NULL, MB_PER_BYTE); //NULL������ ��� �ʱ�ȭ(����Ʈ ����)

	switch (mapping_method) //���� ��Ŀ� ���� ���� ���̺� ũ�� ����
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

		initialized_space_size = MB_PER_BYTE; //���� ����� ���� data_array�� ������
	
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

		initialized_space_size = MB_PER_BYTE; //���� ����� ���� data_array�� ������
		
		break;

	default: //���� ��� ������
		initialized_space_size = MB_PER_BYTE; //���� ����� ���� data_array�� ������
		break;
	}
	
	//�÷��� �޸� ���丮�� ���� ����
	if ((fp = fopen("storage.bin", "wb")) == NULL) //���� + �������� ���
	{
		fprintf(stderr, "storage.bin ������ ������� �� �� �����ϴ�.");
		delete[] data_array; //�޸𸮻󿡼� ����
		return 0;
	}

	//�Է¹��� MB��ŭ ���Ͽ� ���
	init_pos_next = _ftelli64(fp);
	while (1)
	{
		fwrite(data_array, sizeof(char), initialized_space_size, fp); //������ ���� ���� ���
		init_pos_next += initialized_space_size;
		if (init_pos_next >= storage_byte) break; //������ ����� ��ġ�� ����Ʈ������ ȯ���� ��������� �뷮�� ���� ��� ����
	}

	delete[] data_array; //�޸𸮻󿡼� data_array ����

	//�÷��� �޸� �뷮 �� ���� ��� ���
	if ((volume = fopen("volume.txt", "wt")) == NULL) //���� + �ؽ�Ʈ���� ���
	{
		fprintf(stderr, "volume.txt ������ ������� �� �� �����ϴ�.");
		if (fp != NULL)	fclose(fp);
		return 0;
	}
	
	//�÷��� �޸� �뷮 �� ���� ��� ���
	fprintf(volume, "%I64d\n", (*flashmem)->get_flashmem_size());
	fprintf(volume, "%d", mapping_method);

	//��� ������ �������� ���� ���̺�, ���� ���̺� ���
	if ((table = fopen("table.bin", "wb")) == NULL) //���� + �������� ���
	{
		fprintf(stderr, "table.bin ������ ������� �� �� �����ϴ�.");
		if (volume != NULL)	fclose(volume);
		return 0;
	}

	switch (mapping_method) //��� ������ �������� ���� ���̺�, ���� ���̺� ���
	{
	case 0: //���� ���� ���
		fwrite(block_free_space, sizeof(__int64), block_size, table); //��� ������ �������� ���� ���̺� ���
		fwrite(mapping_table, sizeof(__int64), sector_size, table); //���� ���̺� ���
		if ((*flashmem)->block_free_space != NULL && (*flashmem)->mapping_table != NULL) //�ٽ� init�� ��� �ʱ�ȭ �� ���� ����
		{
			//�޸� ����
			delete[] (*flashmem)->block_free_space;
			delete[] (*flashmem)->mapping_table;

			//��ũ �ʱ�ȭ
			(*flashmem)->block_free_space = NULL;
			(*flashmem)->mapping_table = NULL;
		}

		(*flashmem)->block_free_space = block_free_space; //��� ������ �������� ���� ���̺� ����
		(*flashmem)->mapping_table = mapping_table; //���� ���̺� ����
		break;
	case 1: //��� ���� ���
		fwrite(block_free_space, sizeof(__int64), block_size, table); //��� ������ �������� ���� ���̺� ���
		fwrite(mapping_table, sizeof(__int64), block_size, table); //���� ���̺� ���
		if ((*flashmem)->block_free_space != NULL && (*flashmem)->mapping_table != NULL) //�ٽ� init�� ��� �ʱ�ȭ �� ���� ����
		{
			//�޸� ����
			delete[] (*flashmem)->block_free_space;
			delete[] (*flashmem)->mapping_table;

			//��ũ �ʱ�ȭ
			(*flashmem)->block_free_space = NULL;
			(*flashmem)->mapping_table = NULL;
		}
		(*flashmem)->block_free_space = block_free_space; //��� ������ �������� ���� ���̺� ����
		(*flashmem)->mapping_table = mapping_table; //���� ���̺� ����
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

char Flash_read(FlashMem **flashmem, __int64 PSN, int mapping_method) //���� ���Ϳ� �����͸� �о��
{
	FILE *fp = NULL;

	__int64 PBN = 0; //PSN�� ��ġ�ϰ� �ִ� ���� ��� ��ȣ
	__int64 read_pos = 0; //�а��� �ϴ� ���� ������ ��ġ
	char buffer=NULL; //�о���� ������

	if (*flashmem == NULL) //�÷��� �޸𸮰� �Ҵ���� �ʾ��� ���
	{
		std::cout << "not initialized" << std::endl;
		return 0;
	}

	if (PSN > (MB_PER_SECTOR * (*flashmem)->get_flashmem_size()) - 1) //���� �ʰ� ����
	{
		std::cout << "out of range error" << std::endl;
		return 0;
	}

	if ((fp = fopen("storage.bin", "rb")) == NULL) //�б� + �������� ���
	{
		fprintf(stderr, "storage.bin ������ �б���� �� �� �����ϴ�.");
		return 0;
	}

	read_pos = BYTE_PER_SECTOR * PSN; //�а��� �ϴ� ���� ������ ��ġ

	_fseeki64(fp, read_pos, SEEK_SET); //�а��� �ϴ� ���� ������ ��ġ�� �̵�
	fread(&buffer, sizeof(char), 1, fp); //�ش� ���� ���Ϳ� ��ϵ� �� �б�
	fclose(fp);

	if (buffer == NULL)	return '\0';
	else return buffer;
}

int Flash_write(FlashMem **flashmem, __int64 PSN, char data, int mapping_method) //���� ���Ϳ� �����͸� ���
{
	char buffer=NULL;
	__int64 PBN = 0; //PSN�� ��ġ�ϰ� �ִ� ���� ��� ��ȣ
	__int64 write_pos = 0; //������ �ϴ� ��ġ

	//�̹� �Էµ� ��ġ�� ������ �Է� �õ��� overwrite ���� �߻�
	if (*flashmem == NULL) //�÷��� �޸𸮰� �Ҵ���� �ʾ��� ���
	{
		std::cout << "not initialized" << std::endl;
		return 0;
	}

	if (PSN > (MB_PER_SECTOR * (*flashmem)->get_flashmem_size()) - 1) //���� �ʰ� ����
	{
		std::cout << "out of range error" << std::endl;
		return 0;
	}

	FILE *fp = NULL;
	if ((fp = fopen("storage.bin", "rb+")) == NULL) //�а� ���� ��� + �������� ���
	{
		fprintf(stderr, "storage.bin ������ �а� ���� ���� �� �� �����ϴ�.");
		return 0;
	}

	write_pos = BYTE_PER_SECTOR * PSN; //������ �ϴ� ��ġ


	_fseeki64(fp, write_pos, SEEK_SET); //�а��� �ϴ� ���� ������ ��ġ�� �̵�
	fread(&buffer, sizeof(char), 1, fp); //�ش� ���� ���Ϳ� ��ϵ� �� �б�
	_fseeki64(fp, write_pos, SEEK_SET); //������ �ϴ� ���� ������ ��ġ�� �̵�

	if (buffer != NULL) 
	{
		std::cout << "overwrite error" << std::endl;
		fclose(fp);
		return 0;
	}
	else
	{
		fwrite(&data, sizeof(char), 1, fp); //������ ���
		std::cout << "done" << std::endl;
	}

	fclose(fp);
	return 1;
}

int Flash_erase(FlashMem **flashmem, __int64 PBN, int mapping_method) //���� ��Ͽ� �ش��ϴ� �����͸� ����
{
	FILE *fp = NULL;

	char erase_buffer = NULL; //������� �� �� �������� �� ��
	__int64 erase_pos_start = 0; //������� �ϴ� ��� ��ġ�� ���� 
	__int64 erase_pos_end = 0; //������� �ϴ� ��� ��ġ�� ��
	__int64 erase_pos_next = 0; //erase�� ���� ������ ��ġ
	int erase_size = 0; //���� ��Ŀ� ���� erase ���� �� ���� ������ �̵� ������

	//�ش� ����� ���� ���͵鿡 ���ؼ� ��� erase

	if (*flashmem == NULL) //�÷��� �޸𸮰� �Ҵ���� �ʾ��� ���
	{
		std::cout << "not initialized" << std::endl;
		return 0;
	}

	if (PBN > (MB_PER_BLOCK * (*flashmem)->get_flashmem_size()) - 1) //���� �ʰ� ����
	{
		std::cout << "out of range error" << std::endl;
		return 0;
	}

	if ((fp = fopen("storage.bin", "rb+")) == NULL) //�а� ������ + �������� ���(���� �б� ���� �� ��� ���ϳ����� ��� �ʱ�ȭ)
	{
		fprintf(stderr, "storage.bin ������ �а� ���� ���� �� �� �����ϴ�.");
		return 0;
	}


	erase_pos_start = BYTE_PER_BLOCK * PBN; //������� �ϴ� ��� ��ġ�� ���� 
	erase_pos_end = (BYTE_PER_BLOCK * (PBN + 1)) - 1; //������� �ϴ� ��� ��ġ�� ��
	erase_size = BYTE_PER_SECTOR; //���� ��Ŀ� ���� erase ���� �� ���� ������ �̵� ������

	_fseeki64(fp, erase_pos_start, SEEK_SET); //erase�ϰ��� �ϴ� ���� ����� ���� ��ġ�� �̵�
	erase_pos_next = _ftelli64(fp); //erase�� ���� ������ ��ġ
	while(1) //�ش� ��� ��ġ�� ������ �ݺ�
	{
		fwrite(&erase_buffer, sizeof(char), 1, fp);
		erase_pos_next += erase_size;
		if ((erase_pos_next > erase_pos_end)) break;
		else _fseeki64(fp, erase_pos_next, SEEK_SET); //�� ���Ϳ� ���� ����Ʈ ����(512byte)�� �̵�
	}

	//���� ���� �Ǵ� ��� ���� �� Erase ���� ����� �ش� ����� ��� ���� ���� ���̺��� �ʱ�ȭ�ؾ���
	if (mapping_method == 0 || mapping_method == 1)
		(*flashmem)->block_free_space[PBN] = MAPPING_INIT_VALUE;

	fclose(fp);
	std::cout << PBN << "-th block erased" << std::endl;

	return 1;
}