import React, {useRef, useState } from 'react';
import ResizeObserver from 'rc-resize-observer';
import { Layout, Button, Space,  Select, Modal } from 'antd';
import ProCard from '@ant-design/pro-card';
import Editor from "@monaco-editor/react";
import moment from 'moment';

import Graph from './components/AntVGraphin'
import Logger from './util/Logger'
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
  const [output, setOutput] = useState("[INFO] Welcome to c-compiler")
  const [horizontal, setHorizontal] = useState(false)
  const logger = new Logger(output, setOutput)
  function handleEditorDidMount(editor, monaco) {
    editorRef.current = editor;
    editorRef.current.setValue(Examples[0].code)
  }
  
  const handleCompile = async () =>{
    var code = editorRef.current.getValue()
    try {
      let resp = await getAST(code)
      logger.Debug(JSON.stringify(resp))
      if(resp.success){
        logger.Info('Compile success')
        setData(resp.data)
      }
    } catch (error) {
      if(error.response){
        var htmltext = "<div style=\"fontFamily:courier\">"+convert.toHtml(error.response.data)+"</div>"
        logger.Error(htmltext)
        Modal.error({
          title: 'Compile Error!',
          content: parse(htmltext),
          centered:true,
          width: 'max-content',
          bodyStyle:{
            overflowWrap:'normal',
          }
        })
      }
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
      <ResizeObserver onResize={({ width, height }) => {
        if(width < 600)
          setHorizontal(true)
        else
          setHorizontal(false)
      }}>
      <Layout>
         <Header>
         </Header>
         <Content
          style={{
            padding: "20px 20px 0px 20px",
          }}
         >
          <ProCard
            className='card'
            title="Web IDE"
            extra={moment(new Date()).format("YYYY-MM-DD")}
            split={horizontal ? 'horizontal':'vertical'} 
            bordered
            headerBordered
          >
            <ProCard title="Editor"
              className='card'
              extra={
                <Space>
                  <Select defaultValue={1} onChange={handleChange} >
                    {Examples.map(element => <Option key={element.id} value={element.id}>{element.name}</Option>)}
                  </Select>
                  <Button type="primary" onClick={handleCompile}>Compile</Button>
                </Space>
              }
              colSpan={horizontal ? "100%":"25%"}
              bodyStyle={{height:horizontal ?'50vh':'100%'}}
            >
            <Editor
                defaultLanguage='c'
                onMount={handleEditorDidMount}
            />
            </ProCard>
            <ProCard title="Abstract Syntax Tree" 
                    className='card' 
                    extra={
                      <Space>
                        <Button type="primary" onClick={handleSave}>Save</Button>
                        <Button type="primary" onClick={handleRefresh}>Refresh</Button>
                      </Space>
                    }
                    colSpan={horizontal ? "100%":"50%"}
                    >
            <Graph ref={graphRef} data={data}></Graph>
            </ProCard>
            <ProCard title="Log" subTitle="Running Result" className='card' style={{overflow:'auto', maxHeight:"100vh"}} colSpan={horizontal ? "100%":"25%"}>
              {parse(output)}
            </ProCard>
          </ProCard>
        </Content>
        <Footer/>
        </Layout>
      </ResizeObserver>
  );
}

export default App;