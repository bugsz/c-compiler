import moment from 'moment';
export default class Logger {
    constructor(output, f){
        this.output = output
        this.f = f
    }

    timeStampString() {
        return moment(new Date()).format("YYYY/MM/DD - HH:mm:ss")
    }
    
    appendOutput = (data) => {
        this.f(`${this.output}<br/>${data}`)
        console.info(`${this.output}<br/>${data}`)
    }

    Debug(data) {
        const log = `[DEBUG] - ${this.timeStampString()} - ${data}`
        console.log(log)
    }
    
    Info(data) {
        const log = `<span style="color:#3399FF">[INFO] - ${this.timeStampString()} -</span> ${data}`
        console.log(log)
        this.appendOutput(log)
    }

    Warn(data) {
        const log = `[WARN] - ${this.timeStampString()} - ${data}`
        console.warn(log)
        this.appendOutput(log)
    }

    Error(data) {
        const log = `<b><span style="color:#FF0000">[ERROR] - ${this.timeStampString()} -</span></b> ${data}`
        console.error(log)
        this.appendOutput(log)
    }
}