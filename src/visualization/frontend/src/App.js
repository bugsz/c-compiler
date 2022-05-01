import React, {useRef, useState } from 'react';
import { Layout, Button } from 'antd';
import ProCard from '@ant-design/pro-card';
import Editor from "@monaco-editor/react";
import moment from 'moment';

import Graph from './components/AntVGraphin'
import Footer from './components/Footer';
import {getAST, getRunningResult} from './services'
import 'antd/dist/antd.css';
import '@ant-design/pro-card/dist/card.css';
import '@ant-design/pro-layout/dist/layout.css';
import './App.css'

const { Header, Content } = Layout;
function App() {
  const graphRef = useRef(null)
  const editorRef = useRef(null)
  const [data, setData] = useState({id: '1', label: 'c-complier', type: 'circle'})
  function handleEditorDidMount(editor, monaco) {
    editorRef.current = editor; 
  }
  
  const handleCompile = async () =>{
    var code = editorRef.current.getValue()
    let resp = await getAST(code)
    console.info(resp)
    if(resp.success){
      setData(resp.data)
    }
  }

  const handleSave =  () =>{
    graphRef.current.Save()
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
            extra={moment(new Date()).format("YYYY-MM-DD  ")}
            split={'vertical'}
            bordered
            headerBordered
          >
            <ProCard title="Editor" 
              colSpan="33%" 
              className='card' 
              extra={<Button type="primary" onClick={handleCompile}>Compile</Button>}>
              <Editor
                  defaultLanguage='c'
                  defaultValue='int main(){
                                  return 0;
                                }'
                  onMount={handleEditorDidMount}
              />
            </ProCard>
            <ProCard title="Abstract Syntax Tree" 
                    colSpan="33%" 
                    className='card' 
                    extra={<Button type="primary" onClick={handleSave}>Save</Button>}>
            <Graph ref={graphRef} data={data}></Graph>
            </ProCard>
            <ProCard title="Log" subTitle="Running Result" colSpan="33%" className='card'>
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
