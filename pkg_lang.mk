#******************************************************************************
#  Free implementation of Bullfrog's Dungeon Keeper strategy game.
#******************************************************************************
#   @file tool_peresec.mk
#      A script used by GNU Make to recompile the project.
#  @par Purpose:
#      Defines make rules for tools needed to build KeeperFX.
#      Most tools can either by compiled from source or downloaded.
#  @par Comment:
#      None.
#  @author   Tomasz Lis
#  @date     25 Jan 2009 - 02 Jul 2011
#  @par  Copying and copyrights:
#      This program is free software; you can redistribute it and/or modify
#      it under the terms of the GNU General Public License as published by
#      the Free Software Foundation; either version 2 of the License, or
#      (at your option) any later version.
#
#******************************************************************************

NGTEXTDATS = \
pkg/fxdata/gtext_chi.dat \
pkg/fxdata/gtext_cht.dat \
pkg/fxdata/gtext_dut.dat \
pkg/fxdata/gtext_fre.dat \
pkg/fxdata/gtext_ger.dat \
pkg/fxdata/gtext_ita.dat \
pkg/fxdata/gtext_jap.dat \
pkg/fxdata/gtext_pol.dat \
pkg/fxdata/gtext_rus.dat \
pkg/fxdata/gtext_spa.dat \
pkg/fxdata/gtext_swe.dat \
pkg/fxdata/gtext_eng.dat

NCTEXTDATS = \
pkg/campgns/ancntkpr/text_eng.dat \
pkg/campgns/ancntkpr/text_fre.dat \
pkg/campgns/burdnimp/text_eng.dat \
pkg/campgns/dstninja/text_eng.dat \
pkg/campgns/evilkeep/text_eng.dat \
pkg/campgns/evilkeep/text_fre.dat \
pkg/campgns/evilkeep/text_ger.dat \
pkg/campgns/grkreign/text_eng.dat \
pkg/campgns/lqizgood/text_eng.dat \
pkg/campgns/ncastles/text_eng.dat \
pkg/campgns/questfth/text_eng.dat \
pkg/campgns/questfth/text_fre.dat

pkg-languages: $(NGTEXTDATS) $(NCTEXTDATS) pkg-before

# Creation of engine language files from PO/POT files
pkg/fxdata/gtext_jpn.dat: lang/gtext_jpn.po tools/po2ngdat/res/char_encoding_tbl_jp.txt $(POTONGDAT)
	-$(ECHO) 'Building language file: $@'
	$(POTONGDAT) -o "$@" -e "$(word 2,$^)" "$<"
	-$(ECHO) 'Finished building: $@'
	-$(ECHO) ' '

pkg/fxdata/gtext_rus.dat: lang/gtext_rus.po tools/po2ngdat/res/char_encoding_tbl_ru.txt $(POTONGDAT)
	-$(ECHO) 'Building language file: $@'
	$(POTONGDAT) -o "$@" -e "$(word 2,$^)" "$<"
	-$(ECHO) 'Finished building: $@'
	-$(ECHO) ' '

pkg/fxdata/gtext_chi.dat: lang/gtext_chi.po tools/po2ngdat/res/char_encoding_tbl_ch.txt $(POTONGDAT)
	-$(ECHO) 'Building language file: $@'
	$(POTONGDAT) -o "$@" -e "$(word 2,$^)" "$<"
	-$(ECHO) 'Finished building: $@'
	-$(ECHO) ' '

pkg/fxdata/gtext_cht.dat: lang/gtext_cht.po tools/po2ngdat/res/char_encoding_tbl_ch.txt $(POTONGDAT)
	-$(ECHO) 'Building language file: $@'
	$(POTONGDAT) -o "$@" -e "$(word 2,$^)" "$<"
	-$(ECHO) 'Finished building: $@'
	-$(ECHO) ' '

pkg/fxdata/gtext_%.dat: lang/gtext_%.po tools/po2ngdat/res/char_encoding_tbl_eu.txt $(POTONGDAT)
	-$(ECHO) 'Building language file: $@'
	$(POTONGDAT) -o "$@" -e "$(word 2,$^)" "$<"
	-$(ECHO) 'Finished building: $@'
	-$(ECHO) ' '

pkg/fxdata/gtext_%.dat: lang/gtext_%.pot tools/po2ngdat/res/char_encoding_tbl_eu.txt $(POTONGDAT)
	-$(ECHO) 'Building language file: $@'
	$(POTONGDAT) -o "$@" -e "$(word 2,$^)" "$<"
	-$(ECHO) 'Finished building: $@'
	-$(ECHO) ' '

# Creation of engine language files for campaigns
define define_campaign_language_rule
pkg/campgns/$(1)/%.dat: lang/$(1)/%.po tools/po2ngdat/res/char_encoding_tbl_eu.txt $$(POTONGDAT)
	-$$(ECHO) 'Building language file: $$@'
	@$$(MKDIR) $$(@D)
	$$(POTONGDAT) -o "$$@" -e "$$(word 2,$$^)" "$$<"
	-$$(ECHO) 'Finished building: $$@'
	-$$(ECHO) ' '

pkg/campgns/$(1)/%.dat: lang/$(1)/%.pot tools/po2ngdat/res/char_encoding_tbl_eu.txt $$(POTONGDAT)
	-$$(ECHO) 'Building language file: $$@'
	@$$(MKDIR) $$(@D)
	$$(POTONGDAT) -o "$$@" -e "$$(word 2,$$^)" "$$<"
	-$$(ECHO) 'Finished building: $$@'
	-$$(ECHO) ' '

endef

$(foreach campaign,$(sort $(CAMPAIGNS)),$(eval $(call define_campaign_language_rule,$(campaign))))

#******************************************************************************
