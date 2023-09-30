import { addEl, setButtons,fillValues, setValue, buildUrl, fetchJson, setVisible, enableEl, setValues, getParam, fillSelect, forEachEl } from "./helper.js";
import {load as yamlLoad} from "https://cdn.skypack.dev/js-yaml@4.1.0";
(function(){
    const STATUS_INTERVAL=2000;
    const CURRENT_PIPELINE='pipeline';
    let API="cibuild.php";
    let currentPipeline=undefined;
    let downloadUrl=undefined;
    let timer=undefined;
    let structure=undefined;
    let config={};
    let branch=getParam('branch');
    if (! branch) branch='master';
    const showError=(text)=>{
        if (text === undefined){
            setVisible('buildError',false,true);
            return;
        }
        setValue('buildError',text);
        setVisible('buildError',true,true);
    }
    const hideResults = () => {
        downloadUrl = undefined;
        currentPipeline = undefined;
        setValue('pipeline', currentPipeline);
        setValue('status','');
        showError();
        setVisible('download', false, true);
        setVisible('status_url', false, true);
    }
    const setRunning=(active)=>{
        if (active){
            showError();
            downloadUrl=undefined;
            setVisible('download', false, true);
            setVisible('status_url', false, true);
        }
        enableEl('start',!active);
    }
    const fetchStatus=(initial)=>{
        if (currentPipeline === undefined) return;
        fetchJson(API,{api:'status',pipeline:currentPipeline})
                .then((st)=>{
                    setValues(st);
                    setVisible('status_url',st.status_url !== undefined,true);
                    setVisible('error',st.error !== undefined,true);
                    if (st.status === 'error'){
                        setRunning(false);
                        setVisible('download',false,true);
                        return;
                    }
                    if (st.status === 'success'){
                        setRunning(false);
                        fetchJson(API,{api:'artifacts',pipeline:currentPipeline})
                        .then((ar)=>{
                            if (! ar.items || ar.items.length < 1){
                                throw new Error("no download link");
                            }
                            downloadUrl=buildUrl(API,{
                                download: currentPipeline
                            });
                            setVisible('download',true,true);

                        })
                        .catch((err)=>{
                            showError("Unable to get build result: "+err);
                            setVisible('download',false,true);
                        });
                        return;
                    }
                    else{
                        setVisible('download',false,true);
                    }
                    timer=window.setTimeout(fetchStatus,STATUS_INTERVAL)
                })
                .catch((e)=>{
                    timer=window.setTimeout(fetchStatus,STATUS_INTERVAL);
                })
    }
    const setCurrentPipeline=(pipeline)=>{
        currentPipeline=pipeline;
        window.localStorage.setItem(CURRENT_PIPELINE,pipeline);
    };
    const startBuild=()=>{
        let param={'branch':branch};
        currentPipeline=undefined;
        if (timer) window.clearTimeout(timer);
        timer=undefined;
        fillValues(param,['environment','buildflags']);
        setValue('status','requested');
        setValue('pipeline','');
        setRunning(true);
        fetchJson(API,Object.assign({
            api:'start'},param))
        .then((json)=>{
            if (json.status === 'error'){
                throw new Error("unable to create job "+(json.error||''));
            }
            if (!json.id) throw new Error("unable to create job, no id");
            setCurrentPipeline(json.id);
            setValue('pipeline',currentPipeline);
            setValue('status',json.status);
            timer=window.setTimeout(fetchStatus,STATUS_INTERVAL);
        })
        .catch((err)=>{
            setRunning(false);
            setValue('status','error');
            showError(err);
        });
    }
    const runDownload=()=>{
        if (! downloadUrl) return;
        let df=document.getElementById('dlframe');
        if (df){
            df.setAttribute('src',null);
            df.setAttribute('src',downloadUrl);
        }
    }
    const webInstall=()=>{
        if (! downloadUrl) return;
        let url=buildUrl("install.html",{custom:downloadUrl});
        window.location.href=url;
    }
    const btConfig={
        start:startBuild,
        download:runDownload,
        webinstall:webInstall
    };
    const environments=[
        'm5stack-atom-generic',
        'm5stack-atoms3-generic',
        'nodemcu-generic'
    ];
    const loadConfig=async (url)=>{
        let config=await fetch(url).then((r)=>r.text());
        let parsed=yamlLoad(config);
        return parsed;
    }
    const buildSelector=(parent,config,prefix,current,callback)=>{
        let frame=addEl('div','selector',parent);
        let title=addEl('div','title',frame,config.label);
        let name=prefix+"_"+config.key;
        if (! config.values) return;
        config.values.forEach((v)=>{
            let ef=addEl('div','radioFrame',frame);
            addEl('div','label',ef,v.label);
            let re=addEl('input','radioCi',ef);
            re.setAttribute('type','radio');
            re.setAttribute('name',name);
            re.setAttribute('value',v.value);
            re.addEventListener('change',(ev)=>callback(v,ev));
            if (v.description && v.url){
                let lnk=addEl('a','radioDescription',ef,v.description);
                lnk.setAttribute('href',v.url);
                lnk.setAttribute('target','_');
            }
        });
        return frame;
    }
    let selectors=[];
    const buildSelectors=(prefix,configList)=>{
        if (!configList) return;
        let parent=document.getElementById("selectors");
        if (!parent) return;
        let frame=addEl('div','selectorFrame',parent);
        selectors.push(frame);
        let level=selectors.length-1;
        configList.forEach((cfg)=>{
            let name=prefix?(prefix+"_"+cfg.key):cfg.key;
            let current=config[name];
            buildSelector(frame,cfg,name,current,(cfg,ev)=>{
                for (let i=level+1;i<selectors.length;i++){
                    //TODO: remove already set values?
                    selectors[i].remove();
                }
                config[name]=ev.target.value;
                if (cfg.children){
                    buildSelectors(name,cfg.children);
                }
            })
        })
    }
    window.onload=async ()=>{ 
        setButtons(btConfig);
        forEachEl('#environment',(el)=>el.addEventListener('change',hideResults));
        forEachEl('#buildflags',(el)=>el.addEventListener('change',hideResults));
        fillSelect('environment',environments);
        currentPipeline=window.localStorage.getItem(CURRENT_PIPELINE);
        if (currentPipeline){
            setValue('pipeline',currentPipeline);
            fetchStatus(true);
            setRunning(true);
        }
        structure=await loadConfig("testconfig.yaml");
        buildSelectors(undefined,[structure.config.board]);
    }
})();