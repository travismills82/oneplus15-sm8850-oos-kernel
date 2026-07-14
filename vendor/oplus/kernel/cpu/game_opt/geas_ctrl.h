#ifndef _GEAS_CTRL_H_
#define _GEAS_CTRL_H_

enum geas_ctrl_cmd_id {
    UPDATE_GEAS_PARAMS = 1,
    UPDATE_CPU_LPM = 2
};

#define GEAS_MAGIC 0XDF

#define MEM_FALG                                 1
#define BWM_FLAG                                 (1 << 1)
#define FRDR_FLAG                                (1 << 2)
#define GPU_FLAG                                 (1 << 3)
#define EMI_FLAG                                 (1 << 4)
#define NPU_FLAG                                 (1 << 5)

#define DDR_OPP_CNT 10

#define CPU_LPM_RESET                       	 -1

struct frame_drive_params {
	int crc;
	int fd;
	int ei;
	int ais;
	int nais;
	int aas;
	int naas;
	int asis;
	int nasis;
	int asas;
	int nasas;
	int avm;
	int navm;
	int hdr;
	int fdl;
	int sve;
	int sips;
	int smt;
	int lsamplems;
	int dsamplems;
	int liopct;
	int diopct;
	int smz[DDR_OPP_CNT];
	int resv[4];
};

struct gpu_params {
	int imin;
	int imax;
	int amin;
	int amax;
	int ascale;
	int fmin;
	int fmax;
	int resv[4];
};

struct emi_params {
	int opp;
	int resv[4];
};

struct memlat_params {
	int limin;
	int limax;
	int dimin;
	int dimax;
	int resv[4];
};

struct bwmon_params {
	int limin;
	int limax;
	int lascale;
	int lasscale;
	int dimin;
	int dimax;
	int dascale;
	int lsamplems;
	int dsamplems;
	int liopct;
	int diopct;
	int resv[4];
};

struct npu_params {
	int imin;
	int imax;
	int amin;
	int amax;
	int fmin;
	int fmax;
	int resv[4];
};

struct geas_params {
	int geasFlag;
	struct frame_drive_params fdrive_datas;
	struct emi_params emi_datas;
	struct bwmon_params bwmon_datas;
	struct memlat_params memlat_datas;
	struct gpu_params gpu_datas;
	struct npu_params npu_datas;
	int resv[4];
};

#define CMD_ID_UPDATE_GEAS_PARAMS \
	_IOWR(GEAS_MAGIC, UPDATE_GEAS_PARAMS, struct geas_params)

#define CMD_ID_UPDATE_LPM_CPU \
	_IOWR(GEAS_MAGIC, UPDATE_CPU_LPM, int)
int geas_ctrl_init(void);

#endif /* _GEAS_CTRL_H_ */