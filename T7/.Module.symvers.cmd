cmd_/home/pss/Escritorio/T7/Module.symvers :=  sed 's/ko$$/o/'  /home/pss/Escritorio/T7/modules.order | scripts/mod/modpost -m     -o /home/pss/Escritorio/T7/Module.symvers -e -i Module.symvers -T - 
