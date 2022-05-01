import React, {useRef, useState } from 'react';
import { Layout, Button, Space,  Select, Modal } from 'antd';
import ProCard from '@ant-design/pro-card';
import Editor from "@monaco-editor/react";
import moment from 'moment';

import Graph from './components/AntVGraphin'
import Footer from './components/Footer';
import {getAST, getRunningResult} from './services'
import {Examples} from './etc/ExampleCode'
import Convert from 'ansi-to-html';
import parse from 'html-react-parser';

import 'antd/dist/antd.css';
import '@ant-design/pro-card/dist/card.css';
import '@ant-design/pro-layout/dist/layout.css';
import './App.css'

const { Header, Content } = Layout;
const { Option } = Select;
const convert = new Convert({newline: true})
function App() {
  const graphRef = useRef(null)
  const editorRef = useRef(null)
  const [data, setData] = useState({id: '1', label: 'c-complier', type: 'circle'})
  function handleEditorDidMount(editor, monaco) {
    editorRef.current = editor;
    editorRef.current.setValue(Examples[0].code)
  }
  
  const handleCompile = async () =>{
    var code = editorRef.current.getValue()
    try {
      let resp = await getAST(code)
      console.info(resp)
      if(resp.success){
        setData(resp.data)
      }
    } catch (error) {
      console.info(error.response)
      console.info(error.response.data)
      console.info(convert.toHtml(error.response.data))
      const downloadFile = (fileName, data) => { // 保存 string 到 文本文件
          let aLink = document.createElement('a')
          let blob = new Blob([data]); //new Blob([content])
          let evt = document.createEvent("HTMLEvents")
          evt.initEvent("click", true, true); //initEvent 不加后两个参数在FF下会报错  事件类型，是否冒泡，是否阻止浏览器的默认行为
          aLink.download = fileName
          aLink.href = URL.createObjectURL(blob)
          aLink.click()
      }
      downloadFile("1.txt", error.response.data)
      downloadFile("2.txt", convert.toHtml(error.response.data))
      Modal.error({
        title: 'Compile Error!',
        content: parse(convert.toHtml(error.response.data)), 
        centered:true,
        width:"50%"
      })
    }
  }

  const handleChange = (value) =>{
    editorRef.current.setValue(Examples[value-1].code)
  }

  const handleSave = () =>{
    graphRef.current.Save()
  }
  
  const handleRefresh = () =>{
    graphRef.current.Refresh()
  }

  return (
      <>
      <Layout>
         <Header>
         </Header>
         <Content
          style={{
            padding: "20px"
          }}
         >
          <ProCard
            className='card'
            title="Web IDE"
            extra={moment(new Date()).format("YYYY-MM-DD")}
            split={'vertical'}
            bordered
            headerBordered
          >
            <ProCard title="Editor"
              colSpan="25%" 
              className='card' 
              extra={
                <Space>
                  <Select defaultValue={1} onChange={handleChange}>
                    {Examples.map(element => <Option key={element.id} value={element.id}>{element.name}</Option>)}
                  </Select>
                  <Button type="primary" onClick={handleCompile}>Compile</Button>
                </Space>
              }
            >
              <Editor
                  defaultLanguage='c'
                  onMount={handleEditorDidMount}
              />
            </ProCard>
            <ProCard title="Abstract Syntax Tree" 
                    colSpan="60%" 
                    className='card' 
                    extra={
                      <Space>
                        <Button type="primary" onClick={handleSave}>Save</Button>
                        <Button type="primary" onClick={handleRefresh}>Refresh</Button>
                      </Space>
                    }
                    >
            <Graph ref={graphRef} data={data}></Graph>
            </ProCard>
            <ProCard title="Log" subTitle="Running Result" colSpan="15%" className='card'>
              <div>日志</div>
            </ProCard>
          </ProCard>
        </Content>
        <Footer/>
        </Layout>
      </>
  );
}

export default App;