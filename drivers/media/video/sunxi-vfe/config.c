/*
 * set/get config from sysconfig and ini/bin file
 * Author: raymonxiu
 * 
 */


#include "config.h"
#include "cfg_op.h"
#include "camera_detector/camera_detector.h"

#define SIZE_OF_LSC_TBL     7*768*2
#define SIZE_OF_HDR_TBL     4*256*2
#define SIZE_OF_GAMMA_TBL   256*2

#define _SUNXI_CAM_DETECT_

#ifdef _SUNXI_CAM_DETECT_
extern void camera_export_info(char *module_name, int *i2c_addr, int index);
#endif

struct isp_init_config isp_init_def_cfg = {
 /*isp test param */
	  .isp_test_mode = 1,
	  .isp_dbg_level = 0,
	  .isp_focus_len = 100,
	  .isp_gain = 64,  
	  .isp_exp_line = 28336,
	  
	  /*isp enable param */  
	  .sprite_en = 0,
	  .lsc_en = 0,
	  .ae_en = 1,
	  .af_en = 1,
	  .awb_en = 1,
	  .drc_en = 0, 
	  
	  /*maybe change color matrix*/
	  .defog_en = 0,
	  .satur_en = 0,
	  .pri_contrast_en = 0,
	  
	  /*isp tune param */
	  .pri_contrast = 8,  
	  .lsc_center = {1260,950},
	  .vcm_min_code = 10,
	  .vcm_max_code = 1023, 	
	  .bayer_gain_offset = {
	  	256,256,256,256,0,0,0,0
	   },
	  .lsc_tbl = {{0},{0},{0},{0},{0},{0},{0}},
	  .hdr_tbl = {{0},{0},{0},{0}},
	  .gamma_tbl = {0}, 
	  .color_matrix_ini = 
	  {
	    .matrix = {{256,0,0},{0,256,0},{0,0,256}},
        .offset = {0, 0, 0},  	  	
	  },	
};



int fetch_config(struct vfe_dev *dev)
{
#ifndef FPGA_VER  
  int ret;
  unsigned int i;
  char vfe_para[16] = {0};
  char dev_para[16] = {0};
  
  script_item_u   val;
  script_item_value_type_e	type;

  sprintf(vfe_para, "vip%d_para", dev->id);
  /* fetch device quatity issue */
  type = script_get_item(vfe_para,"vip_dev_qty", &val);
  if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {  	
	dev->dev_qty=1;
    vfe_err("fetch csi_dev_qty from sys_config failed\n");
  } else {
	  dev->dev_qty=val.val;
	  vfe_dbg(0,"vip%d_para vip_dev_qty=%d\n",dev->id, dev->dev_qty);
  }

  for(i=0; i<dev->dev_qty; i++)
  {
    /* i2c and module name*/
    sprintf(dev_para, "vip_dev%d_twi_id", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
      vfe_err("fetch vip_dev%d_twi_id from sys_config failed\n", i);
    } else {
      dev->ccm_cfg[i]->twi_id = val.val;
    }
    
    ret = strcmp(dev->ccm_cfg[i]->ccm,"");
    if((dev->ccm_cfg[i]->i2c_addr == 0xff) && (ret == 0)) //when insmod without parm
    {
      sprintf(dev_para, "vip_dev%d_twi_addr", i);
      type = script_get_item(vfe_para, dev_para, &val);
      if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        vfe_err("fetch vip_dev%d_twi_addr from sys_config failed\n", i);
      } else {
        dev->ccm_cfg[i]->i2c_addr = val.val;
      }
      
      sprintf(dev_para, "vip_dev%d_mname", i);
      type = script_get_item(vfe_para, dev_para, &val);
      if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
        char tmp_str[]="ov5650";
        strcpy(dev->ccm_cfg[i]->ccm,tmp_str);
        vfe_err("fetch vip_dev%d_mname from sys_config failed\n", i);
      } else {
        strcpy(dev->ccm_cfg[i]->ccm,val.str);
      }
    }
    
    /* isp used mode */
    sprintf(dev_para, "vip_dev%d_isp_used", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type)
    {
      vfe_dbg(0,"fetch vip_dev%d_isp_used from sys_config failed\n", i);
    } else {
      dev->ccm_cfg[i]->is_isp_used = val.val;
    }
    
    /* fmt */
    sprintf(dev_para, "vip_dev%d_fmt", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
      vfe_dbg(0,"fetch vip_dev%d_fmt from sys_config failed\n", i);
    } else {
      dev->ccm_cfg[i]->is_bayer_raw = val.val;
    }
    
    /* standby mode */
    sprintf(dev_para, "vip_dev%d_stby_mode", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
      vfe_dbg(0,"fetch vip_dev%d_stby_mode from sys_config failed\n", i);
    } else {
      dev->ccm_cfg[i]->power.stby_mode = val.val;
    }
    
    /* fetch flip issue */
    sprintf(dev_para, "vip_dev%d_vflip", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
      vfe_dbg(0,"fetch vip_dev%d_vflip from sys_config failed\n", i);
    } else {
      dev->ccm_cfg[i]->vflip = val.val;
    }
    
    sprintf(dev_para, "vip_dev%d_hflip", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
      vfe_dbg(0,"fetch vip_dev%d_hflip from sys_config failed\n", i);
    } else {
      dev->ccm_cfg[i]->hflip = val.val;
    }
    
    /* fetch power issue*/  
    sprintf(dev_para, "vip_dev%d_iovdd", i);
    type = script_get_item(vfe_para, dev_para, &val);
		
    if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
      char null_str[]="";
      strcpy(dev->ccm_cfg[i]->iovdd_str,null_str);
      vfe_dbg(0,"fetch vip_dev%d_iovdd from sys_config failed\n", i);
    } else {
      strcpy(dev->ccm_cfg[i]->iovdd_str,val.str);
    }
    
    sprintf(dev_para, "vip_dev%d_iovdd_vol", i);
    type = script_get_item(vfe_para,dev_para, &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
	    dev->ccm_cfg[i]->power.iovdd_vol=0;
			vfe_dbg(0,"fetch vip_dev%d_iovdd_vol from sys_config failed, default =0\n",i);
		} else {
	    dev->ccm_cfg[i]->power.iovdd_vol=val.val;
	  }
    
    sprintf(dev_para, "vip_dev%d_avdd", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
      char null_str[]="";
      strcpy(dev->ccm_cfg[i]->avdd_str,null_str);
      vfe_dbg(0,"fetch vip_dev%d_avdd from sys_config failed\n", i);
    } else {
      strcpy(dev->ccm_cfg[i]->avdd_str,val.str);
    }
    
    sprintf(dev_para, "vip_dev%d_avdd_vol", i);
    type = script_get_item(vfe_para,dev_para, &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
	    dev->ccm_cfg[i]->power.avdd_vol=0;
			vfe_dbg(0,"fetch vip_dev%d_avdd_vol from sys_config failed, default =0\n",i);
		} else {
	    dev->ccm_cfg[i]->power.avdd_vol=val.val;
	  }
    
    sprintf(dev_para, "vip_dev%d_dvdd", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_STR != type){
      char null_str[]="";
      strcpy(dev->ccm_cfg[i]->dvdd_str,null_str);
      vfe_dbg(0,"fetch vip_dev%d_dvdd from sys_config failed\n", i);
    } else {
      strcpy(dev->ccm_cfg[i]->dvdd_str, val.str);
    }
		
		sprintf(dev_para, "vip_dev%d_dvdd_vol", i);
    type = script_get_item(vfe_para,dev_para, &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
	    dev->ccm_cfg[i]->power.dvdd_vol=0;
			vfe_dbg(0,"fetch vip_dev%d_dvdd_vol from sys_config failed, default =0\n",i);
		} else {
	    dev->ccm_cfg[i]->power.dvdd_vol=val.val;
	  }
		
    sprintf(dev_para, "vip_dev%d_afvdd", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
      char null_str[]="";
      strcpy(dev->ccm_cfg[i]->afvdd_str,null_str);
      vfe_dbg(0,"fetch vip_dev%d_afvdd from sys_config failed\n", i);
    } else {
      strcpy(dev->ccm_cfg[i]->afvdd_str, val.str);
    }
    
	  sprintf(dev_para, "vip_dev%d_afvdd_vol", i);
    type = script_get_item(vfe_para,dev_para, &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
	    dev->ccm_cfg[i]->power.afvdd_vol=0;
			vfe_dbg(0,"fetch vip_dev%d_afvdd_vol from sys_config failed, default =0\n",i);
		} else {
	    dev->ccm_cfg[i]->power.afvdd_vol=val.val;
	  }
    
    /* fetch reset/power/standby/flash/af io issue */
    sprintf(dev_para, "vip_dev%d_reset", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
      dev->ccm_cfg[i]->gpio.reset_io.gpio = GPIO_INDEX_INVALID;
      vfe_dbg(0,"fetch vip_dev%d_reset from sys_config failed\n", i);
    } else {
      dev->ccm_cfg[i]->gpio.reset_io.gpio=val.gpio.gpio;
      dev->ccm_cfg[i]->gpio.reset_io.mul_sel=val.gpio.mul_sel;
    }
    
    sprintf(dev_para, "vip_dev%d_pwdn", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_PIO != type){
      dev->ccm_cfg[i]->gpio.pwdn_io.gpio = GPIO_INDEX_INVALID;
      vfe_dbg(0,"fetch vip_dev%d_stby from sys_config failed\n", i);
    } else {
      dev->ccm_cfg[i]->gpio.pwdn_io.gpio=val.gpio.gpio;
      dev->ccm_cfg[i]->gpio.pwdn_io.mul_sel=val.gpio.mul_sel;
    }
    sprintf(dev_para, "vip_dev%d_power_en", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
      dev->ccm_cfg[i]->gpio.power_en_io.gpio = GPIO_INDEX_INVALID;
      vfe_dbg(0,"fetch vip_dev%d_power_en from sys_config failed\n", i);
    } else {
      dev->ccm_cfg[i]->gpio.power_en_io.gpio=val.gpio.gpio;
      dev->ccm_cfg[i]->gpio.power_en_io.mul_sel=val.gpio.mul_sel;
    }    
    sprintf(dev_para, "vip_dev%d_flash_en", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
      dev->ccm_cfg[i]->gpio.flash_en_io.gpio = GPIO_INDEX_INVALID;
      dev->ccm_cfg[i]->flash_used=0;
      vfe_dbg(0,"fetch vip_dev%d_flash_en from sys_config failed\n", i);
    } else {
      dev->ccm_cfg[i]->gpio.flash_en_io.gpio=val.gpio.gpio;
      dev->ccm_cfg[i]->gpio.flash_en_io.mul_sel=val.gpio.mul_sel;
      dev->ccm_cfg[i]->flash_used=1;
    }

    sprintf(dev_para, "vip_dev%d_flash_mode", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
      dev->ccm_cfg[i]->gpio.flash_mode_io.gpio = GPIO_INDEX_INVALID;
      vfe_dbg(0,"fetch vip_dev%d_flash_mode from sys_config failed\n", i); 
    } else {
      dev->ccm_cfg[i]->gpio.flash_mode_io.gpio=val.gpio.gpio;
      dev->ccm_cfg[i]->gpio.flash_mode_io.mul_sel=val.gpio.mul_sel;
    }

    sprintf(dev_para, "vip_dev%d_af_pwdn", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
      dev->ccm_cfg[i]->gpio.af_pwdn_io.gpio = GPIO_INDEX_INVALID;

      vfe_dbg(0,"fetch vip_dev%d_af_pwdn from sys_config failed\n", i);
    } else {
      dev->ccm_cfg[i]->gpio.af_pwdn_io.gpio=val.gpio.gpio;
      dev->ccm_cfg[i]->gpio.af_pwdn_io.mul_sel=val.gpio.mul_sel;
    }

		/* fetch actuator issue */
	  sprintf(dev_para, "vip_dev%d_act_used", i);
	  type = script_get_item(vfe_para, dev_para, &val);
	  if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		dev->ccm_cfg[i]->act_used= 0;
		vfe_dbg(0,"fetch vip_dev%d_act_used from sys_config failed\n", i);
	  } else {
		dev->ccm_cfg[i]->act_used=val.val;
	  }
	  
    ret = strcmp(dev->ccm_cfg[i]->act_name,"");
	  if((dev->ccm_cfg[i]->act_slave == 0xff) && (ret == 0)) //when insmod without parm
	  {
  	  sprintf(dev_para, "vip_dev%d_act_name", i);
  	  type = script_get_item(vfe_para, dev_para, &val);
  	    if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
  	      char null_str[]="";
  	      strcpy(dev->ccm_cfg[i]->act_name,null_str);
  	      vfe_dbg(0,"fetch vip_dev%d_act_name from sys_config failed\n", i);
  	    } else {
  	      strcpy(dev->ccm_cfg[i]->act_name,val.str);
  	    }
  		
  		sprintf(dev_para, "vip_dev%d_act_slave", i);
  		type = script_get_item(vfe_para, dev_para, &val);
  		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
  		  dev->ccm_cfg[i]->act_slave= 0;
  		
  		  vfe_dbg(0,"fetch vip_dev%d_act_slave from sys_config failed\n", i);
  		} else {
  		  dev->ccm_cfg[i]->act_slave=val.val;
  		}
	  }
	  else
	  {
	    dev->ccm_cfg[i]->act_used=1;//manual set used
	  }
	  
#ifdef _SUNXI_CAM_DETECT_
	  /*fetch cam detect para*/
		type = script_get_item("camera_list_para", "camera_list_para_used", &val);
    if ((SCIRPT_ITEM_VALUE_TYPE_INT == type) && (val.val == 1)) 
	{
      //camera_export_info(dev->ccm_cfg[i]->ccm, &dev->ccm_cfg[i]->i2c_addr,i);
		unsigned char cam_name[20];
		unsigned int address;
		camera_export_info(cam_name, &address,i);

		if( (strcmp(cam_name,"")!=0)&&(address!=0) )
		{
			strcpy(dev->ccm_cfg[i]->ccm, cam_name);
			dev->ccm_cfg[i]->i2c_addr=address;
		}
		else
		{
			vfe_warn("detect none sensor in list, use sysconfig setting!\n");
		}
    }
#endif
  //vfe_dbg(0,"act_used=%d, name=%s, slave=0x%x\n",dev->ccm_cfg[0]->act_used,
  //	dev->ccm_cfg[0]->act_name, dev->ccm_cfg[0]->act_slave);
  }
#else
  int type;
  unsigned int i2c_addr_vip0[2] = {0x6c,0x00};
  unsigned int i2c_addr_vip1[2] = {0x78,0x42};
  unsigned int i;
  unsigned char ccm_vip0_dev0[] = {"ov8825",};
  unsigned char ccm_vip0_dev1[] = {"",};
  unsigned char ccm_vip1_dev0[] = {"ov5650",};
  unsigned char ccm_vip1_dev1[] = {"gc0308",};  
  unsigned int i2c_addr[2];
  unsigned char *ccm_name[2];
  
  if(dev->id==0) {
    dev->dev_qty = 1; 
    i2c_addr[0] = i2c_addr_vip0[0];
    i2c_addr[1] = i2c_addr_vip0[1];
    ccm_name[0] = ccm_vip0_dev0;
    ccm_name[1] = ccm_vip0_dev1;
  } else if (dev->id == 1) {
    dev->dev_qty = 1;
    i2c_addr[0] = i2c_addr_vip1[0];
    i2c_addr[1] = i2c_addr_vip1[1];
    ccm_name[0] = ccm_vip1_dev0;
    ccm_name[1] = ccm_vip1_dev1;
  }
  
  for(i=0; i<dev->dev_qty; i++)
  { 
    dev->ccm_cfg[i]->twi_id = 1;
    type = strcmp(dev->ccm_cfg[i]->ccm,"");
    if((dev->ccm_cfg[i]->i2c_addr == 0xff) && (ret == 0)) //when insmod without parm
    {
      dev->ccm_cfg[i]->i2c_addr = i2c_addr[i];
      strcpy(dev->ccm_cfg[i]->ccm, ccm_name[i]);
    }
    dev->ccm_cfg[i]->power.stby_mode = 0;
    dev->ccm_cfg[i]->vflip = 0;
    dev->ccm_cfg[i]->hflip = 0;
  }
#endif

  for(i=0; i<dev->dev_qty; i++)
  {
    vfe_dbg(0,"dev->ccm_cfg[%d]->ccm = %s\n",i,dev->ccm_cfg[i]->ccm);
    vfe_dbg(0,"dev->ccm_cfg[%d]->twi_id = %x\n",i,dev->ccm_cfg[i]->twi_id);
    vfe_dbg(0,"dev->ccm_cfg[%d]->i2c_addr = %x\n",i,dev->ccm_cfg[i]->i2c_addr);
    vfe_dbg(0,"dev->ccm_cfg[%d]->vflip = %x\n",i,dev->ccm_cfg[i]->vflip);
    vfe_dbg(0,"dev->ccm_cfg[%d]->hflip = %x\n",i,dev->ccm_cfg[i]->hflip);
    vfe_dbg(0,"dev->ccm_cfg[%d]->iovdd_str = %s\n",i,dev->ccm_cfg[i]->iovdd_str);
    vfe_dbg(0,"dev->ccm_cfg[%d]->avdd_str = %s\n",i,dev->ccm_cfg[i]->avdd_str);
    vfe_dbg(0,"dev->ccm_cfg[%d]->dvdd_str = %s\n",i,dev->ccm_cfg[i]->dvdd_str);
    vfe_dbg(0,"dev->ccm_cfg[%d]->afvdd_str = %s\n",i,dev->ccm_cfg[i]->afvdd_str);
    vfe_dbg(0,"dev->ccm_cfg[%d]->act_used = %d\n",i,dev->ccm_cfg[i]->act_used);
    vfe_dbg(0,"dev->ccm_cfg[%d]->act_name = %s\n",i,dev->ccm_cfg[i]->act_name);
    vfe_dbg(0,"dev->ccm_cfg[%d]->act_slave = 0x%x\n",i,dev->ccm_cfg[i]->act_slave);
  }

  return 0;
}

int fetch_isp_cfg(struct vfe_dev *dev, struct cfg_section *cfg_section)
{
  int j,type,len,isp_id;  
  struct cfg_subkey subkey;  
  short *matrix,*offset;
  char sub_name[30] = {0};
  char *buf;  
  buf = (char*)kzalloc(SIZE_OF_LSC_TBL,GFP_KERNEL);
  isp_id = 0;
  /* fetch ISP isp_test_mode! */
  type = cfg_get_one_subkey(cfg_section,"isp_test","isp_test_mode",&subkey);
  if (CFG_ITEM_VALUE_TYPE_INT != type)
  {  
    vfe_dbg(0,"fetch isp_test_mode from camera.ini failed,apply default value!\n");
	dev->isp_gen_set[isp_id].isp_ini_cfg.isp_test_mode = 0;
  }
  else
  {    
    dev->isp_gen_set[isp_id].isp_ini_cfg.isp_test_mode=subkey.value->val;
	vfe_dbg(0,"isp_ini_cfg.isp_test_mode = %d\n",dev->isp_gen_set[isp_id].isp_ini_cfg.isp_test_mode);
  }
  if(dev->isp_gen_set[isp_id].isp_ini_cfg.isp_test_mode != 0)
  {
    
	/* fetch ISP isp_dbg_level! */
	type = cfg_get_one_subkey(cfg_section,"isp_test","isp_dbg_level",&subkey);
	if (CFG_ITEM_VALUE_TYPE_INT != type)
	{  
	  vfe_err("fetch isp_dbg_level from camera.ini failed,apply default value!\n");
	  dev->isp_gen_set[isp_id].isp_ini_cfg.isp_dbg_level = 0;
	}
	else
	{	 
	  dev->isp_gen_set[isp_id].isp_ini_cfg.isp_dbg_level=subkey.value->val;
	  vfe_dbg(0,"isp_ini_cfg.isp_dbg_level = %d\n",dev->isp_gen_set[isp_id].isp_ini_cfg.isp_dbg_level);
	}
	
	/* fetch ISP isp_focus_len! */
	type = cfg_get_one_subkey(cfg_section,"isp_test","isp_focus_len",&subkey);
	if (CFG_ITEM_VALUE_TYPE_INT != type)
	{  
	  vfe_err("fetch isp_focus_len from camera.ini failed,apply default value!\n");
	  dev->isp_gen_set[isp_id].isp_ini_cfg.isp_focus_len = 0;
	}
	else
	{	 
	  dev->isp_gen_set[isp_id].isp_ini_cfg.isp_focus_len=subkey.value->val;
	  vfe_dbg(0,"isp_ini_cfg.isp_focus_len = %d\n",dev->isp_gen_set[isp_id].isp_ini_cfg.isp_focus_len);
	}
	/* fetch ISP isp_exp_line! */
	type = cfg_get_one_subkey(cfg_section,"isp_test","isp_exp_line",&subkey);
	if (CFG_ITEM_VALUE_TYPE_INT != type)
	{  
	  vfe_err("fetch isp_exp_line from camera.ini failed,apply default value!\n");
	  dev->isp_gen_set[isp_id].isp_ini_cfg.isp_exp_line = 0;
	}
	else
	{	 
	  dev->isp_gen_set[isp_id].isp_ini_cfg.isp_exp_line=subkey.value->val;
	  vfe_dbg(0,"isp_ini_cfg.isp_exp_line = %d\n",dev->isp_gen_set[isp_id].isp_ini_cfg.isp_exp_line);
	}
	/* fetch ISP isp_gain! */
	type = cfg_get_one_subkey(cfg_section,"isp_test","isp_gain",&subkey);
	if (CFG_ITEM_VALUE_TYPE_INT != type)
	{  
	  vfe_err("fetch isp_gain from camera.ini failed,apply default value!\n");
	  dev->isp_gen_set[isp_id].isp_ini_cfg.isp_gain = 16;
	}
	else
	{	 
	  dev->isp_gen_set[isp_id].isp_ini_cfg.isp_gain=subkey.value->val;
	  vfe_dbg(0,"isp_ini_cfg.isp_gain = %d\n",dev->isp_gen_set[isp_id].isp_ini_cfg.isp_gain);
	}
	
  }
  else
  {
	dev->isp_gen_set[isp_id].isp_ini_cfg.isp_dbg_level     = 0;
	dev->isp_gen_set[isp_id].isp_ini_cfg.isp_focus_len	   = 100;
	dev->isp_gen_set[isp_id].isp_ini_cfg.isp_gain          = 1;	
	dev->isp_gen_set[isp_id].isp_ini_cfg.isp_exp_line      = 1771*16;
  }

  
  /* fetch ISP enable! */
  
  /* fetch sprite_en enable! */
  type = cfg_get_one_subkey(cfg_section,"isp_func","sprite_en",&subkey);
  if (CFG_ITEM_VALUE_TYPE_INT != type)
  {  
    vfe_err("fetch sprite_en from camera.ini failed,apply default value!\n");	
    dev->isp_gen_set[isp_id].isp_ini_cfg.sprite_en = 0;
  }
  else
  {
    dev->isp_gen_set[isp_id].isp_ini_cfg.sprite_en =subkey.value->val;
	vfe_dbg(0,"isp_ini_cfg.sprite_en = %d\n",dev->isp_gen_set[isp_id].isp_ini_cfg.sprite_en);
  }
  /* fetch satur_en enable! */
  type = cfg_get_one_subkey(cfg_section,"isp_func","satur_en",&subkey);
  if (CFG_ITEM_VALUE_TYPE_INT != type)
  {  
    vfe_err("fetch satur_en from camera.ini failed,apply default value!\n");
	dev->isp_gen_set[isp_id].isp_ini_cfg.satur_en = 0;
  }
  else
  {
    
    dev->isp_gen_set[isp_id].isp_ini_cfg.satur_en = subkey.value->val;
	vfe_dbg(0,"isp_ini_cfg.satur_en = %d\n",dev->isp_gen_set[isp_id].isp_ini_cfg.satur_en);
  }
  
  /* fetch pri_contrast_en enable! */
  type = cfg_get_one_subkey(cfg_section,"isp_func","pri_contrast_en",&subkey);
  if (CFG_ITEM_VALUE_TYPE_INT != type)
  {  
    vfe_err("fetch pri_contrast_en from camera.ini failed,apply default value!\n");
	dev->isp_gen_set[isp_id].isp_ini_cfg.pri_contrast_en = 0;
  }
  else
  {
    
    dev->isp_gen_set[isp_id].isp_ini_cfg.pri_contrast_en = subkey.value->val;
	vfe_dbg(0,"isp_ini_cfg.pri_contrast_en = %d\n",dev->isp_gen_set[isp_id].isp_ini_cfg.pri_contrast_en);
  }

  /* fetch lsc_en enable! */
  type = cfg_get_one_subkey(cfg_section,"isp_func","lsc_en",&subkey);
  if (CFG_ITEM_VALUE_TYPE_INT != type)
  {  
    vfe_err("fetch lsc_en from camera.ini failed,apply default value!\n");
	dev->isp_gen_set[isp_id].isp_ini_cfg.lsc_en = 0;
  }
  else
  {
    
    dev->isp_gen_set[isp_id].isp_ini_cfg.lsc_en=subkey.value->val;
	vfe_dbg(0,"isp_ini_cfg.lsc_en = %d\n",dev->isp_gen_set[isp_id].isp_ini_cfg.lsc_en);
  }
  /* fetch high_quality_mode_en! */
  type = cfg_get_one_subkey(cfg_section,"isp_func","high_quality_mode_en",&subkey);
  if (CFG_ITEM_VALUE_TYPE_INT != type)
  {  
    vfe_err("fetch high_quality_mode_en from camera.ini failed,apply default value!\n");
	dev->isp_gen_set[isp_id].isp_ini_cfg.high_quality_mode_en = 0;
  }
  else
  {    
    dev->isp_gen_set[isp_id].isp_ini_cfg.high_quality_mode_en=subkey.value->val;
	vfe_dbg(0,"isp_ini_cfg.high_quality_mode_en = %d\n",dev->isp_gen_set[isp_id].isp_ini_cfg.high_quality_mode_en);
  }
  
  /* fetch defog_en enable! */
  type = cfg_get_one_subkey(cfg_section,"isp_func","defog_en",&subkey);
  
  if (CFG_ITEM_VALUE_TYPE_INT != type)
  {  
    vfe_err("fetch defog_en from camera.ini failed,apply default value!\n");	
    dev->isp_gen_set[isp_id].isp_ini_cfg.defog_en = 1;
  }
  else
  {
    dev->isp_gen_set[isp_id].isp_ini_cfg.defog_en =subkey.value->val;
	vfe_dbg(0,"isp_ini_cfg.defog_en = %d\n",dev->isp_gen_set[isp_id].isp_ini_cfg.defog_en);
  }
  /* fetch ae_en enable! */
  type = cfg_get_one_subkey(cfg_section,"isp_func","ae_en",&subkey);
  
  if (CFG_ITEM_VALUE_TYPE_INT != type)
  {  
    vfe_err("fetch ae_en from camera.ini failed,apply default value!\n");	
    dev->isp_gen_set[isp_id].isp_ini_cfg.ae_en = 0;
  }
  else
  {
    dev->isp_gen_set[isp_id].isp_ini_cfg.ae_en =subkey.value->val;
	vfe_dbg(0,"isp_ini_cfg.ae_en = %d\n",dev->isp_gen_set[isp_id].isp_ini_cfg.ae_en);
  }
  /* fetch af_en enable! */
  type = cfg_get_one_subkey(cfg_section,"isp_func","af_en",&subkey);
  if (CFG_ITEM_VALUE_TYPE_INT != type)
  {  
    vfe_err("fetch af_en from camera.ini failed,apply default value!\n");	
    dev->isp_gen_set[isp_id].isp_ini_cfg.af_en = 0;
  }
  else
  {
    dev->isp_gen_set[isp_id].isp_ini_cfg.af_en =subkey.value->val;
	vfe_dbg(0,"isp_ini_cfg.af_en = %d\n",dev->isp_gen_set[isp_id].isp_ini_cfg.af_en);
  }
  /* fetch awb_en enable! */
  type = cfg_get_one_subkey(cfg_section,"isp_func","awb_en",&subkey);
  if (CFG_ITEM_VALUE_TYPE_INT != type)
  {  
    vfe_err("fetch awb_en from camera.ini failed,apply default value!\n");	
    dev->isp_gen_set[isp_id].isp_ini_cfg.awb_en = 0;
  }
  else
  {
    dev->isp_gen_set[isp_id].isp_ini_cfg.awb_en =subkey.value->val;
	vfe_dbg(0,"isp_ini_cfg.awb_en = %d\n",dev->isp_gen_set[isp_id].isp_ini_cfg.awb_en);
  }
  /* fetch drc_en enable! */
  type = cfg_get_one_subkey(cfg_section,"isp_func","drc_en",&subkey);
  if (CFG_ITEM_VALUE_TYPE_INT != type)
  {  
    vfe_err("fetch drc_en from camera.ini failed,apply default value!\n");	
    dev->isp_gen_set[isp_id].isp_ini_cfg.drc_en = 0;
  }
  else
  {
    dev->isp_gen_set[isp_id].isp_ini_cfg.drc_en =subkey.value->val;
	vfe_dbg(0,"isp_ini_cfg.drc_en = %d\n",dev->isp_gen_set[isp_id].isp_ini_cfg.drc_en);
  }
  /* fetch ISP VCM range! */
  type = cfg_get_one_subkey(cfg_section,"isp_vcm","vcm_min_code",&subkey);
  if (CFG_ITEM_VALUE_TYPE_INT != type)
  {  
    vfe_err("fetch vcm_min_code from camera.ini failed,apply default value!\n");	
    dev->isp_gen_set[isp_id].isp_ini_cfg.vcm_min_code = 0;
  }
  else
  {
    dev->isp_gen_set[isp_id].isp_ini_cfg.vcm_min_code =subkey.value->val;
	vfe_dbg(0,"isp_ini_cfg.vcm_min_code = %d\n",dev->isp_gen_set[isp_id].isp_ini_cfg.vcm_min_code);
  }
  
  type = cfg_get_one_subkey(cfg_section,"isp_vcm","vcm_max_code",&subkey);
  if (CFG_ITEM_VALUE_TYPE_INT != type)
  {  
    vfe_err("fetch vcm_max_code from camera.ini failed,apply default value!\n");	
    dev->isp_gen_set[isp_id].isp_ini_cfg.vcm_max_code = 1023;
  }
  else
  {
    dev->isp_gen_set[isp_id].isp_ini_cfg.vcm_max_code =subkey.value->val;
	vfe_dbg(0,"isp_ini_cfg.vcm_max_code = %d\n",dev->isp_gen_set[isp_id].isp_ini_cfg.vcm_max_code);
  }
  /* fetch pri_contrast! */
  type = cfg_get_one_subkey(cfg_section,"isp_func","pri_contrast",&subkey);
  if (CFG_ITEM_VALUE_TYPE_INT != type)
  {  
    vfe_err("fetch pri_contrast from camera.ini failed,apply default value!\n");	
    dev->isp_gen_set[isp_id].isp_ini_cfg.pri_contrast = 0;
  }
  else
  {
    dev->isp_gen_set[isp_id].isp_ini_cfg.pri_contrast =subkey.value->val;
	vfe_dbg(0,"isp_ini_cfg.pri_contrast = %d\n",dev->isp_gen_set[isp_id].isp_ini_cfg.pri_contrast);
  }
  
  /* fetch denoise_level! */
  type = cfg_get_one_subkey(cfg_section,"isp_func","denoise_level",&subkey);
  if (CFG_ITEM_VALUE_TYPE_INT != type)
  {  
    vfe_err("fetch denoise_level from camera.ini failed,apply default value!\n");	
    dev->isp_gen_set[isp_id].isp_ini_cfg.denoise_level = 0;
  }
  else
  {
    dev->isp_gen_set[isp_id].isp_ini_cfg.denoise_level =subkey.value->val;
	vfe_dbg(0,"isp_ini_cfg.denoise_level = %d\n",dev->isp_gen_set[isp_id].isp_ini_cfg.denoise_level);
  }
  
  /* fetch sharpness_level! */
  type = cfg_get_one_subkey(cfg_section,"isp_func","sharpness_level",&subkey);
  if (CFG_ITEM_VALUE_TYPE_INT != type)
  {  
    vfe_err("fetch sharpness_level from camera.ini failed,apply default value!\n");	
    dev->isp_gen_set[isp_id].isp_ini_cfg.sharpness_level = 0;
  }
  else
  {
    dev->isp_gen_set[isp_id].isp_ini_cfg.sharpness_level =subkey.value->val;
	vfe_dbg(0,"isp_ini_cfg.sharpness_level = %d\n",dev->isp_gen_set[isp_id].isp_ini_cfg.sharpness_level);
  }

  /* fetch defog_value! */
  type = cfg_get_one_subkey(cfg_section,"isp_func","defog_value",&subkey);
  if (CFG_ITEM_VALUE_TYPE_INT != type)
  {
    vfe_err("fetch defog_value from camera.ini failed,apply default value!\n");
    dev->isp_gen_set[isp_id].isp_ini_cfg.defog_value = 60;
  }
  else
  {
    dev->isp_gen_set[isp_id].isp_ini_cfg.defog_value =subkey.value->val;
	vfe_dbg(0,"isp_ini_cfg.sharpness_level = %d\n",dev->isp_gen_set[isp_id].isp_ini_cfg.defog_value);
  }

  /* fetch gain_delay_frame! */
  type = cfg_get_one_subkey(cfg_section,"isp_func","gain_delay_frame",&subkey);
  if (CFG_ITEM_VALUE_TYPE_INT != type)
  {
    vfe_err("fetch gain_delay_frame from camera.ini failed,apply default value!\n");
    dev->isp_gen_set[isp_id].isp_ini_cfg.gain_delay_frame = 1;
  }
  else
  {
    dev->isp_gen_set[isp_id].isp_ini_cfg.gain_delay_frame =subkey.value->val;
	vfe_dbg(0,"isp_ini_cfg.sharpness_level = %d\n",dev->isp_gen_set[isp_id].isp_ini_cfg.gain_delay_frame);
  }

  
  /* fetch ISP table! */
  
  /* fetch gamma_tbl table! */
  type = cfg_get_one_subkey(cfg_section,"isp_tbl","gamma_tbl",&subkey);
  if (CFG_ITEM_VALUE_TYPE_STR != type)
  { 
    vfe_err("fetch gamma_tbl from camera.ini failed!\n");
  }
  else
  {
  
    vfe_dbg(0,"isp_ini_cfg.gamma_tbl = %s\n",subkey.value->str);
	len = cfg_read_file(subkey.value->str,buf,SIZE_OF_GAMMA_TBL);
	if(len < 0)
	{
	  vfe_err("read gamma_tbl from gamma_tbl.bin failed!\n");
	}
	else
	{
	  memcpy(dev->isp_gen_set[isp_id].isp_ini_cfg.gamma_tbl, buf, len);
	}
  }

  /* fetch lsc table! */
  if(dev->isp_gen_set[isp_id].isp_ini_cfg.lsc_en == 1)
  {
	  type = cfg_get_one_subkey(cfg_section,"isp_tbl","lsc_tbl",&subkey);
	  if (CFG_ITEM_VALUE_TYPE_STR != type)
	  {  
		vfe_err("fetch lsc_tbl from camera.ini failed!\n");
	  }
	  else
	  {
	  
	    vfe_dbg(0,"isp_ini_cfg.lsc_tbl = %s\n",subkey.value->str);
		len = cfg_read_file(subkey.value->str,buf,SIZE_OF_LSC_TBL);
		if(len < 0)
		{
			vfe_err("read lsc_tbl from lsc_tbl.bin failed!\n");
		}
		else
		{
		  memcpy(dev->isp_gen_set[isp_id].isp_ini_cfg.lsc_tbl, buf, len);
		}
	  }	  
  }
  /* fetch hdr_tbl table!*/ 
  type = cfg_get_one_subkey(cfg_section,"isp_tbl","hdr_tbl",&subkey);
  if (CFG_ITEM_VALUE_TYPE_STR != type)
  {  
    vfe_err("fetch lsc_tbl from camera.ini failed!\n");
  }
  else
  {
  
    vfe_dbg(0,"isp_ini_cfg.hdr_tbl = %s\n",subkey.value->str);
	len = cfg_read_file(subkey.value->str,buf,SIZE_OF_HDR_TBL);
	if(len < 0)
	{
		vfe_err("read hdr_tbl from hdr_tbl.bin failed!\n");
	}
	else
	{
	  memcpy(dev->isp_gen_set[isp_id].isp_ini_cfg.hdr_tbl, buf, len);
	}	
  }

  /* fetch ISP lens center! */  
  type = cfg_get_one_subkey(cfg_section,"isp_lsc","lsc_center_x",&subkey);
  if (CFG_ITEM_VALUE_TYPE_INT != type)
  {  
    vfe_err("fetch lsc_center_x from camera.ini failed!\n");
	dev->isp_gen_set[isp_id].isp_ini_cfg.lsc_center[0] = dev->width/2;
  }
  else
  {
    dev->isp_gen_set[isp_id].isp_ini_cfg.lsc_center[0]=subkey.value->val;
	
	vfe_dbg(0,"isp_ini_cfg..lsc_center[0] = %d\n",dev->isp_gen_set[isp_id].isp_ini_cfg.lsc_center[0]);
  }
  type = cfg_get_one_subkey(cfg_section,"isp_lsc","lsc_center_y",&subkey);
  if (CFG_ITEM_VALUE_TYPE_INT != type)
  {  
    vfe_err("fetch lsc_center_y from camera.ini failed!\n");
	
	dev->isp_gen_set[isp_id].isp_ini_cfg.lsc_center[1] = dev->height/2;
  }
  else
  {
    dev->isp_gen_set[isp_id].isp_ini_cfg.lsc_center[1]=subkey.value->val;
	
	vfe_dbg(0,"isp_ini_cfg..lsc_center[1] = %d\n",dev->isp_gen_set[isp_id].isp_ini_cfg.lsc_center[1]);
  }
  
  /* fetch ISP bayer gain offset! */ 
  for(j = 0;j<8;j++)
  {
	sprintf(sub_name, "gain_offset_%d", j);
	  type = cfg_get_one_subkey(cfg_section,"isp_bayer_gain_offset",sub_name,&subkey);
	  if (CFG_ITEM_VALUE_TYPE_INT != type)
	  {  
		vfe_err("fetch gain_offset_%d from camera.ini failed,apply default value!\n",j);
		if(j<4)
		{
			dev->isp_gen_set[isp_id].isp_ini_cfg.bayer_gain_offset[j]=256;
		}
		else
		{
			dev->isp_gen_set[isp_id].isp_ini_cfg.bayer_gain_offset[j]=0;
		}
	  }
	  else
	  {
		dev->isp_gen_set[isp_id].isp_ini_cfg.bayer_gain_offset[j]=subkey.value->val;
		
		vfe_dbg(0,"isp_ini_cfg.isp_ini_cfg.bayer_gain_offset[%d] = %d\n",j,dev->isp_gen_set[isp_id].isp_ini_cfg.bayer_gain_offset[j]);
	  }
  }
  
   /* fetch ISP color matrix! */ 
   matrix = (short*)dev->isp_gen_set[isp_id].isp_ini_cfg.color_matrix_ini.matrix;
   offset = (short*)dev->isp_gen_set[isp_id].isp_ini_cfg.color_matrix_ini.offset;
   for(j = 0;j<9;j++)
   {
	 sprintf(sub_name, "matrix_%d", j);
	   type = cfg_get_one_subkey(cfg_section,"isp_color_matrix",sub_name,&subkey);
	   if (CFG_ITEM_VALUE_TYPE_INT != type)
	   {  
		 vfe_err("fetch matrix_%d from camera.ini failed!\n",j);
	   }
	   else
	   {	    
		 matrix[j]=subkey.value->val;
		 
		 vfe_dbg(0,"matrix[%d] = %d\n",j,matrix[j]);
	   }
   }   
   for(j = 0;j<3;j++)
   {
	 sprintf(sub_name, "offset_%d", j);
	   type = cfg_get_one_subkey(cfg_section,"isp_color_matrix",sub_name,&subkey);
	   if (CFG_ITEM_VALUE_TYPE_INT != type)
	   {  
		 vfe_err("fetch offset_%d from camera.ini failed!\n",j);
	   }
	   else
	   {
		 offset[j]=subkey.value->val;
	   }
   }
  if(buf)
  {
  	kfree(buf);
  }
   vfe_dbg(0,"fetch isp_cfg done!\n");
  return 0;  
}

int read_ini_info(struct vfe_dev *dev)
{
  char path[128] = "/system/etc/hawkview/camera.ini";
  int ret = 0,isp_id = 0;
  struct cfg_section *cfg_section;
  char *buf;  
  buf = (char*)kzalloc(2048,GFP_KERNEL);
  
  vfe_dbg(0,"read ini start\n");
  cfg_section_init(&cfg_section);
  ret = cfg_read_ini(path, &cfg_section);
  
  if(ret == -1)
  {
    dev->isp_gen_set[isp_id].isp_ini_cfg = isp_init_def_cfg;
  	goto read_ini_info_end;
  }
  
  fetch_isp_cfg(dev, cfg_section);
  
read_ini_info_end:
  cfg_section_release(&cfg_section); 
  vfe_dbg(0,"read ini end\n");
  if(buf)
    kfree(buf);
  return ret;
}

