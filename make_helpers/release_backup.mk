OPEN_HEADER_FILE+=plat/ecnt/en7523/include/asm/types.h
OPEN_HEADER_FILE+=plat/ecnt/en7523/include/linux/version.h

BACKUP_DIR=./backup

$(eval BACKUP_HEADER_FILE := $(addprefix $(BACKUP_DIR)/,$(OPEN_HEADER_FILE)))
$(eval BACKUP_HEADER_DIR := $(dir $(BACKUP_HEADER_FILE)))

$(BACKUP_HEADER_FILE): $(OPEN_HEADER_FILE)

$(BACKUP_DIR)/%: %
	cp -rf $< $@

ecnt_prepare:
	mkdir -pv $(BACKUP_HEADER_DIR)

ecnt_backup: ecnt_prepare $(BACKUP_HEADER_FILE)
	tar cvzf backup.tar.gz $(BACKUP_DIR)
	rm -rf $(BACKUP_DIR)
	@echo "Backup released source code successfully"

ecnt_restore:
	tar xzvf backup.tar.gz
	cp -rf $(BACKUP_DIR)/* ./
	rm -rf $(BACKUP_DIR) backup.tar.gz
	@echo "Restore released source code successfully"

