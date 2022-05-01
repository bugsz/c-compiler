import axios from 'axios';
import settings from './etc/settings.json';
const url = settings.url;

const getAST = async (code) => {
    let res = await axios(
            url+'/GetAST',
            {
                method: "POST",
                data: code
            }
        )
    return {
        success: res.status === 200,
        data: res.data
    }
}

const getRunningResult = async (code) => {
    let res = await axios(
            url+'/GetRunningResult',
            {
                method: "POST",
                data: code,
            }
        )
    return {
        success: res.status === 200,
        data: res.data
    }
}

export{
    getAST,
    getRunningResult
}