#include "header.h"

FlashMem *flash = NULL;

extern int mapping_method = 2; //���� �÷��� �޸��� ���ι�� (default : 2 - ������� ����)

void main() {
	flash->chk_initialized(&flash,mapping_method); //�̹� ������ �÷��� �޸𸮰� �ִ��� Ȯ��

	while (1)
	{
		flash->disp_command(mapping_method); //Ŀ�ǵ� ���
		flash->disp_flash_info(&flash, mapping_method); //�÷��� �޸� ���� ���
		flash->input_command(&flash,mapping_method); //Ŀ�ǵ� �Է�
	}
}