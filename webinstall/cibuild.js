import { addEl, setButtons,fillValues, setValue, buildUrl, fetchJson, setVisible, enableEl, setValues, getParam, fillSelect, forEachEl, readFile } from "./helper.js";
import {load as yamlLoad} from "https://cdn.skypack.dev/js-yaml@4.1.0";
import fileDownload from "https://cdn.skypack.dev/js-file-download@0.4.12"
(function(){
    const STATUS_INTERVAL=2000;
    const CURRENT_PIPELINE='pipeline';
    const API="cibuild.php";
    const GITAPI="install.php";
    const GITUSER="wellenvogel";
    const GITREPO="esp32-nmea2000";
    let currentPipeline=undefined;
    let downloadUrl=undefined;
    let timer=undefined;
    let structure=undefined;
    let config={}; //values as read and stored
    let configStruct={}; //complete struct merged of config and struct
    let displayMode='last';
    let delayedSearch=undefined;
    let running=false;
    let pipelineState=undefined;
    let gitSha=undefined;
    const modeStrings={
        last: 'Last Build',
        existing: 'Existing Build',
        current: 'Current Build'
    };
    const setDisplayMode=(mode)=>{
        let old=displayMode;
        let ms=modeStrings[mode];
        if (ms === undefined){
            return false;
        }
        displayMode=mode;
        setValue('resultTitle',ms);
        return mode !== old;
    }
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
    const updateStart=()=>{
        if (running) {
            enableEl('start',false);
            return;
        }
        let e=document.getElementById('configError');
        if (e.textContent) {
            enableEl('start',false);
            return;
        }
        if (displayMode != 'existing'){
            if (currentPipeline !== undefined){
                //check pipeline state
                if (['error','success'].indexOf(pipelineState) >= 0){
                    enableEl('start',true);
                    return;       
                }
                enableEl('start',false);
                return;    
            }
            enableEl('start',true);
        }
        //display mnode existing
        enableEl('start',currentPipeline === undefined);
    }
    const setRunning=(active)=>{
        running=active;
        if (active){
            showError();
            downloadUrl=undefined;
            setVisible('download', false, true);
            setVisible('status_url', false, true);
        }
        updateStart();
    }
    const isRunning=()=>{
        return running;
    }
    const fetchStatus=(initial)=>{
        if (currentPipeline === undefined) {
            setVisible('status_url',false,true);
            setVisible('error',false);
            setVisible('download',false,true);
            setValue('status','---');
            updateStart();
            return;
        };
        let queryPipeline=currentPipeline;
        fetchJson(API,{api:'status',pipeline:currentPipeline})
                .then((st)=>{
                    if (queryPipeline !== currentPipeline) return;
                    if (st.status === undefined) st.status=st.state;
                    pipelineState=st.status;
                    setValues(st);
                    setVisible('status_url',st.status_url !== undefined,true);
                    setVisible('error',st.error !== undefined,true);
                    if (st.status === 'error' || st.status === 'errored'){
                        setRunning(false);
                        setVisible('download',false,true);
                        updateStart();
                        return;
                    }
                    if (st.status === 'success'){
                        setRunning(false);
                        updateStart();
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
                    updateStart();
                    timer=window.setTimeout(fetchStatus,STATUS_INTERVAL)
                })
                .catch((e)=>{
                    timer=window.setTimeout(fetchStatus,STATUS_INTERVAL);
                })
    }
    const setCurrentPipeline=(pipeline,doStore)=>{
        currentPipeline=pipeline;
        setValue('pipeline',currentPipeline);
        if (doStore) window.localStorage.setItem(CURRENT_PIPELINE,pipeline);
    };
    const startBuild=()=>{
        let param={};
        currentPipeline=undefined;
        if (timer) window.clearTimeout(timer);
        timer=undefined;
        fillValues(param,['environment','buildflags']);
        setValue('status','requested');
        setValue('pipeline','');
        setRunning(true);
        updateStart();
        if (gitSha !== undefined) param.tag=gitSha;
        param.config=JSON.stringify(config);
        fetchJson(API,Object.assign({
            api:'start'},param))
        .then((json)=>{
            if (json.status === 'error'){
                throw new Error("unable to create job "+(json.error||''));
            }
            if (!json.id) throw new Error("unable to create job, no id");
            setCurrentPipeline(json.id,true);
            setValue('status',json.status);
            setDisplayMode('current');
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
    const uploadConfig=()=>{
       let form=document.getElementById("upload");
       form.reset();
       let fsel=document.getElementById("fileSelect");
       fsel.onchange=async ()=>{
            if (fsel.files.length < 1) return;
            let file=fsel.files[0];
            if (! file.name.match(/json$/)){
                alert("only json files");
                return;
            }
            try{
                let content=await readFile(file,true);
                config=JSON.parse(content);
                buildSelectors(ROOT_PATH,structure.config.children,true);
                findPipeline();
            } catch (e){
                alert("upload "+fsel.files[0].name+" failed: "+e);
            }

       }
       fsel.click(); 
    }
    const downloadConfig=()=>{
        let name="buildconfig.json";
        fileDownload(JSON.stringify(config),name);
    }
    const btConfig={
        start:startBuild,
        download:runDownload,
        webinstall:webInstall,
        uploadConfig: uploadConfig,
        downloadConfig: downloadConfig
    };
    const loadConfig=async (url)=>{
        let config=await fetch(url).then((r)=>{
            if (!r.ok) throw new Error("unable to fetch: "+r.statusText);
            return r.text()
        });
        let parsed=yamlLoad(config);
        return parsed;
    }
    const getVal=(cfg,keys)=>{
        for (let i in keys){
            let k=cfg[keys[i]];
            if (k !== undefined) return k;
        }
    }
    const KEY_NAMES=['key','value','label'];
    const LABEL_NAMES=['label','value'];
    const PATH_ATTR='data-path';
    const SEPARATOR=':';
    /**
     * 
     * @param {build a selector} parent 
     * @param {*} config 
     * @param {*} name 
     * @param {*} current 
     * @param {*} callback will be called with: children,key,value,initial
     * @returns 
     */
    const buildSelector=(parent,config,name,current,callback)=>{
        let rep=new RegExp("[^"+SEPARATOR+"]*","g");
        let level=name.replace(rep,'');
        let frame=addEl('div','selector level'+level.length,parent);
        frame.setAttribute(PATH_ATTR,name);
        let title=addEl('div','title',frame,config.label);
        if (config.type === 'select') {
            if (!config.values) return;
            config.values.forEach((v) => {
                let ef = addEl('div', 'radioFrame', frame);
                addEl('div', 'label', ef, getVal(v, LABEL_NAMES));
                let re = addEl('input', 'radioCi', ef);
                let val = v.value;
                let key=getVal(v,KEY_NAMES);
                if (val === undefined) val=key;
                re.setAttribute('type', 'radio');
                re.setAttribute('name', name);
                re.addEventListener('change', (ev) => callback(v.children,key,val,v.resource,false));
                if (v.description && v.url) {
                    let lnk = addEl('a', 'radioDescription', ef, v.description);
                    lnk.setAttribute('href', v.url);
                    lnk.setAttribute('target', '_');
                }
                if (key == current) {
                    re.setAttribute('checked','checked');
                    callback(v.children,key,val,v.resource,true);
                }
            });
        }
        return frame;
    }
    const removeSelectors=(prefix,removeValues)=>{
        forEachEl('.selectorFrame',(el)=>{
            let path=el.getAttribute(PATH_ATTR);
            if (! path) return;
            if (path.indexOf(prefix) == 0){
                el.remove();
            }
        })
        if (removeValues){
            let removeKeys=[];
            for (let k in configStruct){
                if (k.indexOf(prefix) == 0) removeKeys.push(k);
            }
            for (let k in config){
                if (k.indexOf(prefix) == 0) removeKeys.push(k);
            }
            removeKeys.forEach((k)=>{
                delete config[k];
                delete configStruct[k];
            });
        }
    }
    const buildSelectors=(prefix,configList,initial)=>{
        removeSelectors(prefix,!initial);
        if (!configList) return;
        let parent=document.getElementById("selectors");
        if (!parent) return;
        let frame=addEl('div','selectorFrame',parent);
        frame.setAttribute(PATH_ATTR,prefix);
        configList.forEach((cfg)=>{
            let name=prefix?(prefix+SEPARATOR+cfg.key):cfg.key;
            let current=config[name];
            buildSelector(frame,cfg,name,current,(children,key,value,resource,initial)=>{
                buildSelectors(name,children,initial);
                configStruct[name]={cfg:cfg, key: key, value:value,resource:resource};
                buildValues(initial);
            })
        })
    }
    const ROOT_PATH='root';
    const buildValues=(initial)=>{
        let environment;
        let flags="";
        if (! initial){
            config={};
        }
        let allowedResources={};
        let currentResources={};
        let errors="";
        for (let round = 0; round <= 1; round++) {
            //round1: find allowed resources
            //round2: really collect values
            for (let k in configStruct) {
                let struct = configStruct[k];
                if (!struct || !struct.cfg || struct.value === undefined) continue;
                if (round > 0) config[k] = struct.key;
                if (struct.cfg.target !== undefined) {
                    if (struct.cfg.target === 'environment') {
                        if (round > 0) environment = struct.value;
                        else allowedResources=struct.resource;
                        continue;
                    }
                    if (round < 1) continue;
                    if (struct.resource){
                        let resList=currentResources[struct.resource];
                        if (! resList){
                            resList=[];
                            currentResources[struct.resource]=resList;
                        }
                        resList.push(struct);
                    }
                    if (struct.cfg.target === 'define') {
                        flags += " -D" + struct.value;
                        continue;
                    }
                    const DEFPRFX = "define:";
                    if (struct.cfg.target.indexOf(DEFPRFX) == 0) {
                        let def = struct.cfg.target.substring(DEFPRFX.length);
                        flags += " -D" + def + "=" + struct.value;
                        continue;
                    }
                }
            }
        }
        document.getElementById('environment').value=environment;
        document.getElementById('buildflags').value=flags;
        //check resources
        for (let k in currentResources){
            let resList=currentResources[k];
            if (allowedResources[k] !== undefined){
                if (resList.length > allowedResources[k]){
                    errors+=" more than "+allowedResources[k]+" "+k+" device(s) used";
                }
            }
        }
        if (errors){
            setValue('configError',errors);
            setVisible('configError',true,true);
        }
        else{
            setValue('configError','');
            setVisible('configError',false,true);
        }
        if (! initial) findPipeline();
        updateStart();
    }
    let findIdx=0;
    const findPipeline=()=>{
        if (delayedSearch !== undefined){
            window.clearTimeout(delayedSearch);
            delayedSearch=undefined;
        }
        if (isRunning()) {
            delayedSearch=window.setTimeout(findPipeline,500);
            return;
        }
        findIdx++;
        let queryIdx=findIdx;
        let param={find:1};
        fillValues(param,['environment','buildflags']);
        if (gitSha !== undefined) param.tag=gitSha;
        fetchJson(API,param)
            .then((res)=>{
                if (queryIdx != findIdx) return;
                setCurrentPipeline(res.pipeline);
                updateStart();
                setDisplayMode('existing');
                fetchStatus(true); 
            })
            .catch((e)=>console.log("findPipeline error ",e));

    }
    window.onload=async ()=>{ 
        setButtons(btConfig);
        forEachEl('#environment',(el)=>el.addEventListener('change',hideResults));
        forEachEl('#buildflags',(el)=>el.addEventListener('change',hideResults));
        let pipeline=window.localStorage.getItem(CURRENT_PIPELINE);
        setDisplayMode('last');
        if (pipeline){
            setCurrentPipeline(pipeline);
            fetchStatus(true);
            setRunning(true);
        }
        let gitParam={user:GITUSER,repo:GITREPO};
        let branch=getParam('branch');
        if (branch){
            try{
                let info=await fetchJson(GITAPI,Object.assign({},gitParam,{branch:branch}));
                if (info.object){
                    gitSha=info.object.sha;
                    setValue('branchOrTag','branch');
                    setValue('branchOrTagValue',branch);
                }
            }catch (e){
                console.log("branch query error",e);
            }
        }
        if (gitSha === undefined) {
            let tag = getParam('tag');
            let type="tag";
            if (!tag) {
                try {
                    let relinfo = await fetchJson(GITAPI, Object.assign({}, gitParam, { api: 1 }));
                    if (relinfo.tag_name) {
                        tag = relinfo.tag_name;
                        type="release";
                    }
                    else {
                        alert("unable to query latest release");
                    }
                } catch (e) {
                    alert("unable to query release info " + e);
                }
            }
            if (tag){
                try{
                    let info=await fetchJson(GITAPI,Object.assign({},gitParam,{tag:tag}));
                    if (info.object){
                        gitSha=info.object.sha;
                        setValue('branchOrTag',type);
                        setValue('branchOrTagValue',tag);    
                    }
                }catch(e){
                    alert("cannot get sha for tag "+tag+": "+e);
                }
            }
        }
        if (gitSha === undefined){
            //last resort: no sha, let the CI pick up latest
            setValue('gitSha','unknown');
            setValue('branchOrTag','branch');
            setValue('branchOrTagValue','master');
        }
        else{
            setValue('gitSha',gitSha);
        }
        if (gitSha !== undefined){
            let url=buildUrl(GITAPI,Object.assign({},gitParam,{sha:gitSha,proxy:'webinstall/build.yaml'}));
            try{
                structure=await loadConfig(url);
            }catch (e){
                alert("unable to load config for selected release:\n "+e+"\n falling back to default");
            }
        }
        if (! structure){
            structure=await loadConfig("build.yaml");
        }
        buildSelectors(ROOT_PATH,structure.config.children,true);
        if (! running) findPipeline();
        updateStart();
    }
})();